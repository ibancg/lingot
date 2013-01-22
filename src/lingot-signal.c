/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
 *
 * This file is part of lingot.
 *
 * lingot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * lingot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <math.h>

#include "lingot-signal.h"
#include "lingot-complex.h"

/*
 peak identification functions.
 */

static int lingot_signal_is_peak(const FLT* signal, int index,
		short peak_half_width) {
	register unsigned int j;

	for (j = 0; j < peak_half_width; j++) {
		if ((signal[index + j] < signal[index + j + 1])
				|| (signal[index - j] < signal[index - j - 1])) {
			return 0;
		}
	}
	return 1;
}

static FLT lingot_signal_fft_bin_interpolate_quinn2_tau(FLT x) {
	return (0.25 * log(3 * x * x + 6 * x + 1)
			- 0.102062072615966
					* log(
							(x + 1 - 0.816496580927726)
									/ (x + 1 + 0.816496580927726)));
}

static FLT lingot_signal_fft_bin_interpolate_quinn2(LingotComplex y1,
		LingotComplex y2, LingotComplex y3) {
	FLT absy2_2 = y2[0] * y2[0] + y2[1] * y2[1];
	FLT ap = (y3[0] * y2[0] + y3[1] * y2[1]) / absy2_2;
	FLT dp = -ap / (1.0 - ap);
	FLT am = (y1[0] * y2[0] + y1[1] * y2[1]) / absy2_2;
	FLT dm = am / (1.0 - am);
	return (dp + dm) / 2 + lingot_signal_fft_bin_interpolate_quinn2_tau(dp * dp)
			- lingot_signal_fft_bin_interpolate_quinn2_tau(dm * dm);
}

// for qsort
static int lingot_signal_compare_int(const void *a, const void *b) {
	return (*((int*) a) < *((int*) b)) ? -1 : 1;
}

// Returns a factor to multiply with in order to give more importance to higher
// frequency harmonics. This is to give more importance to the higher divisors
// for the same selected sets.
static FLT lingot_signal_frequency_penalty(FLT freq) {
	static const FLT f0 = 100;
	static const FLT f1 = 1000;
	static const FLT alpha0 = 0.99;
	static const FLT alpha1 = 1;
	const FLT freqPenaltyA = (alpha0 - alpha1) / (f0 - f1);
	const FLT freqPenaltyB = -(alpha0 * f1 - f0 * alpha1) / (f0 - f1);

	return freq * freqPenaltyA + freqPenaltyB;
}

// search the fundamental peak given the spd and its 2nd derivative
FLT lingot_signal_estimate_fundamental_frequency(const FLT* spl,
		LingotComplex* const fft, const FLT* noise, int N, int n_peaks,
		int lowest_index, short peak_half_width, FLT delta_f_fft, FLT min_snr,
		FLT min_q, FLT min_freq, LingotCore* core, short* divisor) {
	register unsigned int i, j, m;
	int p_index[n_peaks];

	// at this moment there are no peaks.
	for (i = 0; i < n_peaks; i++)
		p_index[i] = -1;

	if (lowest_index < peak_half_width) {
		lowest_index = peak_half_width;
	}

	short n_found_peaks = 0;

	// I'll get the n_peaks maximum peaks with SNR above the required.
	for (i = lowest_index; i < N - peak_half_width; i++)
		if ((spl[i] - noise[i] > min_snr)
				&& lingot_signal_is_peak(spl, i, peak_half_width)) {

			// search a place in the maximums buffer, if it doesn't exists, the
			// lower maximum is candidate to be replaced.
			m = 0;				// first candidate.
			for (j = 0; j < n_peaks; j++) {
				if (p_index[j] == -1) {
					m = j; // there is a place.
					break;
				}

				// if there is no place, finds the smallest peak as a candidate
				// to be substituted.
				if (spl[p_index[j]] < spl[p_index[m]]) {
					m = j;
				}
			}

			if (p_index[m] == -1) {
				p_index[m] = i; // there is a place
			} else if (spl[i] > spl[p_index[m]]) {
				p_index[m] = i; // if greater
			}

			if (n_found_peaks < n_peaks) {
				n_found_peaks++;
			}
		}

	qsort(p_index, n_found_peaks, sizeof(int), &lingot_signal_compare_int);

	FLT freq_interpolated[n_found_peaks];
	for (i = 0; i < n_found_peaks; i++) {
		freq_interpolated[i] = delta_f_fft
				* (p_index[i]
						+ lingot_signal_fft_bin_interpolate_quinn2(
								fft[p_index[i] - 1], fft[p_index[i]],
								fft[p_index[i] + 1]));
	}

	// maximum ratio error
	static const FLT ratioTol = 0.05;
	static const short max_divisor = 4;

	short tone_index = 0;
	short div = 0;
	FLT groundFreq = 0.0;
	FLT ratios[n_found_peaks];
	FLT error[n_found_peaks];
	short indices_related[n_found_peaks];
	short n_indices_related = 0;

	FLT bestQ = 0.0;
	short best_indices_related[n_found_peaks];
	FLT bestF = 0;
	short bestDivisor = 1;

	// possible ground frequencies
	for (tone_index = 0; tone_index < n_found_peaks; tone_index++) {
		for (div = 1; div <= max_divisor; div++) {
			groundFreq = freq_interpolated[tone_index] / div;
			if (groundFreq > min_freq) {
				n_indices_related = 0;
				for (i = 0; i < n_found_peaks; i++) {
					ratios[i] = freq_interpolated[i] / groundFreq;
					error[i] = (ratios[i] - round(ratios[i]));
					// harmonically related frequencies
					if (fabs(error[i]) < ratioTol) {
						indices_related[n_indices_related++] = i;
					}
				}

				// add the penalties for short sets and high divisors
				FLT q = 0.0;
				for (i = 0; i < n_indices_related; i++) {
					// add up contributions to the quality factor
					q += (spl[p_index[indices_related[i]]]
							- noise[p_index[indices_related[i]]])
							* lingot_signal_frequency_penalty(groundFreq);
				}

				FLT maxFreq =
						freq_interpolated[indices_related[n_indices_related - 1]];
				FLT f = maxFreq;

//				printf(
//						"tone = %i, divisor = %i, gf = %f, q = %f, n = %i, f = %f, ",
//						tone, divisor, groundFreq, q, n_indices_related, f);
//				for (i = 0; i < n_indices_related; i++) {
//					printf("%f ", freq_interpolated[indices_related[i]]);
//				}
//				printf("\n");

				if (q > bestQ) {
					bestQ = q;
					memcpy(best_indices_related, indices_related,
							n_indices_related * sizeof(short));
					bestDivisor = round(f / groundFreq);
					bestF = f;
				}

			} else {
				break;
			}
		}
	}

//	printf("BEST selected f = %f, f* = %f, div = %i, q = %f, n = %i, ",
//			bestF / bestDivisor, bestDivisor, bestF, bestQ,
//			best_n_indices_related);
//	for (i = 0; i < best_n_indices_related; i++) {
//		printf("%f ", freq_interpolated[best_indices_related[i]]);
//	}
//	printf("\n");

	if ((bestF != 0.0) && (bestQ < min_q)) {
		bestF = 0.0;
//		printf("q < mq!!\n");
	}

	if (bestF == 0.0) {
		core->markers_size = 0;
	}

	*divisor = bestDivisor;
	return bestF;

}

void lingot_signal_compute_noise_level(const FLT* spd, int N, int cbuffer_size,
		FLT* noise_level) {

	const int n = cbuffer_size;
	const int _np2 = n / 2;
	const FLT _1pn = 1.0 / n;
	FLT cbuffer[n];

	int i = 0;
	unsigned char cbuffer_top = n - 1;

	// moving average filter

	// first sample
	FLT spli = spd[0];
	FLT cbuffer_sum = spli * n;
	for (i = 0; i < n; i++) {
		cbuffer[i] = spli;
	}

	// O(n) algorithm
	for (i = -_np2; i <= N + _np2; i++) {
		if (i + _np2 < N) {
			spli = spd[i + _np2]; // new sample
		}

		cbuffer_top++;
		if (cbuffer_top >= n) {
			cbuffer_top = 0;
		}
		cbuffer_sum -= cbuffer[cbuffer_top]; // exiting sample
		cbuffer[cbuffer_top] = spli; // new sample in the buffer
		cbuffer_sum += spli;

		if ((i >= 0) && (i < N)) {
			noise_level[i] = cbuffer_sum * _1pn;
		}
	}
}

//---------------------------------------------------------------------------

// generates a N-samples window
void lingot_signal_window(int N, FLT* out, window_type_t window_type) {
	register int i;
	switch (window_type) {
	case HANNING:
		for (i = 0; i < N; i++)
			out[i] = 0.5 * (1 - cos((2.0 * M_PI * i) / (N - 1)));
		break;
	case HAMMING:
		for (i = 0; i < N; i++)
			out[i] = 0.53836 - 0.46164 * cos((2.0 * M_PI * i) / (N - 1));
		break;
	default:
		break;
	}
}

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

FLT lingot_signal_get_noise_threshold(LingotConfig* conf, FLT w) {
	//return 0.5*(1.0 - 0.9*w/M_PI);
	return pow(10.0, (conf->noise_threshold_db * (1.0 - 0.9 * w / M_PI)) / 10.0);
	//return conf->noise_threshold_un;
}

//---------------------------------------------------------------------------

/* returns the index of the maximun of the buffer x of size N */FLT lingot_signal_get_max(
		const FLT *x, int N, int* Mi) {
	register int i;
	FLT M;

	M = -1.0;
	*Mi = -1;

	for (i = 0; i < N; i++) {

		if (x[i] > M) {
			M = x[i];
			*Mi = i;
		}
	}

	return M;
}

// search the fundamental peak given the spd and its 2nd derivative
int lingot_signal_get_fundamental_peak(const LingotConfig* conf, FLT *x,
		FLT* d2x, int N) {
	register unsigned int i, j, m;
	int p_index[conf->peak_number];

	// at this moment there is no peaks.
	for (i = 0; i < conf->peak_number; i++)
		p_index[i] = -1;

	unsigned int lowest_index = (unsigned int) ceil(
			conf->min_frequency * (1.0 * conf->oversampling / conf->sample_rate)
					* conf->fft_size);

	if (lowest_index < conf->peak_half_width)
		lowest_index = conf->peak_half_width;

	// I'll get the PEAK_NUMBER maximum peaks.
	for (i = lowest_index; i < N - conf->peak_half_width; i++)
		if (lingot_signal_is_peak(x, i, conf->peak_half_width)) {

			// search a place in the maximums buffer, if it doesn't exists, the
			// lower maximum is candidate to be replaced.
			m = 0; // first candidate.
			for (j = 0; j < conf->peak_number; j++) {
				if (p_index[j] == -1) {
					m = j; // there is a place.
					break;
				}

				if (d2x[p_index[j]] < d2x[p_index[m]])
					m = j; // search the lowest.
			}

			if (p_index[m] == -1)
				p_index[m] = i; // there is a place
			else if (d2x[i] > d2x[p_index[m]])
				p_index[m] = i; // if greater
		}

	FLT maximum = 0.0;
	int maximum_index = -1;

	// search the maximum peak
	for (i = 0; i < conf->peak_number; i++)
		if ((p_index[i] != -1) && (x[p_index[i]] > maximum)) {
			maximum = x[p_index[i]];
			maximum_index = p_index[i];
		}

	if (maximum_index == -1)
		return N;

	// all peaks much lower than maximum are deleted.
	for (i = 0; i < conf->peak_number; i++)
		if ((p_index[i] == -1)
				|| (conf->peak_rejection_relation_nu * x[p_index[i]] < maximum))
			p_index[i] = N; // there are available places in the buffer.

	// search the lowest maximum index.
	for (m = 0, j = 0; j < conf->peak_number; j++) {
		if (p_index[j] < p_index[m])
			m = j;
	}

	return p_index[m];
}

static FLT lingot_signal_fft_bin_interpolate_quinn2_tau(FLT x) {
	return (0.25 * log(3 * x * x + 6 * x + 1.0)
			- 0.102062072615966
					* log(
							(x + 1.0 - 0.816496580927726)
									/ (x + 1.0 + 0.816496580927726)));
}

static FLT lingot_signal_fft_bin_interpolate_quinn2(LingotComplex y1,
		LingotComplex y2, LingotComplex y3) {
	FLT absy2_2 = y2[0] * y2[0] + y2[1] * y2[1];
	FLT ap = (y3[0] * y2[0] + y3[1] * y2[1]) / absy2_2;
	FLT dp = -ap / (1.0 - ap);
	FLT am = (y1[0] * y2[0] + y1[1] * y2[1]) / absy2_2;
	FLT dm = am / (1.0 - am);
	return 0.5 * (dp + dm)
			+ lingot_signal_fft_bin_interpolate_quinn2_tau(dp * dp)
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

	// TODO: precompute
	const FLT freqPenaltyA = (alpha0 - alpha1) / (f0 - f1);
	const FLT freqPenaltyB = -(alpha0 * f1 - f0 * alpha1) / (f0 - f1);

	return freq * freqPenaltyA + freqPenaltyB;
}

// search the fundamental peak given the spd and its 2nd derivative
FLT lingot_signal_estimate_fundamental_frequency(const FLT* snr, FLT freq,
		LingotComplex* const fft, int N, int n_peaks, int lowest_index,
		int highest_index, short peak_half_width, FLT delta_f_fft, FLT min_snr,
		FLT min_q, FLT min_freq, LingotCore* core, short* divisor) {
	register unsigned int i, j, m;
	int p_index[n_peaks];
	FLT magnitude[n_peaks];

#ifdef DRAW_MARKERS
	core->markers_size = 0;
#endif

	// at this moment there are no peaks.
	for (i = 0; i < n_peaks; i++)
		p_index[i] = -1;

	if (lowest_index < peak_half_width) {
		lowest_index = peak_half_width;
	}
	if (highest_index > N - peak_half_width) {
		highest_index = N - peak_half_width;
	}

	short n_found_peaks = 0;

	// I'll get the n_peaks maximum peaks with SNR above the required.
	for (i = lowest_index; i < highest_index; i++) {

		FLT factor = 1.0;

		if (freq != 0.0) {

			FLT f = i * delta_f_fft;
			if (fabs(f / freq - round(f / freq)) < 0.07) { // TODO: tune and put in conf
				factor = 1.5; // TODO: tune and put in conf
			}

		}

		FLT snri = snr[i] * factor;

		if ((snri > min_snr)
				&& lingot_signal_is_peak(snr, i, peak_half_width)) {

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
				if (magnitude[j] < magnitude[m]) {
					m = j;
				}
			}

			if (p_index[m] == -1) {
				p_index[m] = i; // there is a place
				magnitude[m] = snri;
			} else if (snr[i] > snr[p_index[m]]) {
				p_index[m] = i; // if greater
				magnitude[m] = snri;
			}

			if (n_found_peaks < n_peaks) {
				n_found_peaks++;
			}
		}
	}

	if (n_found_peaks == 0) {
		return 0.0;
	}

	FLT maximum = 0.0;

	// search the maximum peak
	for (i = 0; i < n_found_peaks; i++)
		if ((p_index[i] != -1) && (magnitude[i] > maximum)) {
			maximum = magnitude[i];
		}

	// all peaks much lower than maximum are deleted.
	int delete_counter = 0;
	for (i = 0; i < n_found_peaks; i++) {
		if ((p_index[i] == -1) || (magnitude[i] < maximum - 20.0)) { // TODO: conf
			p_index[i] = N; // there are available places in the buffer.
			delete_counter++;
		}
	}

	qsort(p_index, n_found_peaks, sizeof(int), &lingot_signal_compare_int);
	n_found_peaks -= delete_counter;

	FLT freq_interpolated[n_found_peaks];
	FLT delta = 0.0;
	for (i = 0; i < n_found_peaks; i++) {
		delta = lingot_signal_fft_bin_interpolate_quinn2(fft[p_index[i] - 1],
				fft[p_index[i]], fft[p_index[i] + 1]);
		freq_interpolated[i] = delta_f_fft * (p_index[i] + delta);

#ifdef DRAW_MARKERS
		core->markers[core->markers_size++] = p_index[i];
#endif
	}

// maximum ratio error
	static const FLT ratioTol = 0.02; // TODO: tune
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
//				int highest_harmonic_index = n_indices_related - 1;
				int highest_harmonic_index = 0;
				FLT highest_harmonic_magnitude = 0.0;
				FLT q = 0.0;
				FLT f = 0.0;
				FLT snrsum = 0.0;
				for (i = 0; i < n_indices_related; i++) {
					// add up contributions to the quality factor
					q += snr[p_index[indices_related[i]]]
							* lingot_signal_frequency_penalty(groundFreq);
					if (snr[p_index[indices_related[i]]]
							> highest_harmonic_magnitude) {
						highest_harmonic_index = i;
						highest_harmonic_magnitude =
								snr[p_index[indices_related[i]]];
					}

//					snrsum += snr[p_index[indices_related[i]]];
//					short myDivisor = round(
//							freq_interpolated[indices_related[i]] / groundFreq);
//					f += snr[p_index[indices_related[i]]]
//							* freq_interpolated[indices_related[i]] / myDivisor;
				}

				f = freq_interpolated[indices_related[highest_harmonic_index]];

//				f /= snrsum;

//				printf(">>>>>>>>>< f = %f\n", f);

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

#ifdef DRAW_MARKERS
					core->markers_size2 = 0;
					for (i = 0; i < n_indices_related; i++) {
						core->markers2[core->markers_size2++] =
								p_index[indices_related[i]];
					}
#endif

//					printf("%d\n", n_indices_related);
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

#ifdef DRAW_MARKERS
	if (bestF == 0.0) {
		core->markers_size2 = 0;
	}
#endif

	*divisor = bestDivisor;
	return bestF;

}

void lingot_signal_compute_noise_level(const FLT* spd, int N, int cbuffer_size,
		FLT* noise_level) {

// low pass IIR filter.
	const FLT c = 0.1;
	const FLT filter_a[] = { 1.0, c - 1.0 };
	const FLT filter_b[] = { c };
	static LingotFilter* filter = 0x0;

	if (filter == 0x0) {
		filter = lingot_filter_new(1, 0, filter_a, filter_b);
	}

	lingot_filter_reset(filter);

	lingot_filter_filter(filter, cbuffer_size, spd, noise_level);
	lingot_filter_filter(filter, N, spd, noise_level);

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

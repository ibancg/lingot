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

/* returns the maximun index of buffer x (size N) */
void lingot_signal_get_max(const FLT *x, int N, int* Mi) {
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
}

//---------------------------------------------------------------------------

int lingot_signal_is_peak(const FLT* signal, int index, short peak_half_width,
		FLT noise) {
	register unsigned int j;

	// a peak must be greater than noise threshold.
	if (signal[index] < noise)
		return 0;

	for (j = 0; j < peak_half_width; j++) {
		if (signal[index + j] < signal[index + j + 1])
			return 0;
		if (signal[index - j] < signal[index - j - 1])
			return 0;
	}
	return 1;
}

//---------------------------------------------------------------------------

// search the fundamental peak given the spd and its 2nd derivative
int lingot_signal_get_fundamental_peak(const LingotConfig* conf, const FLT *x,
		const FLT* d2x, int N) {
	register unsigned int i, j, m;
	int p_index[conf->peak_number];

	// at this moment there are no peaks.
	for (i = 0; i < conf->peak_number; i++)
		p_index[i] = -1;

	unsigned int lowest_index = (unsigned int) ceil(
			conf->min_frequency * (1.0 * conf->oversampling / conf->sample_rate)
					* conf->fft_size);

	if (lowest_index < conf->peak_half_width)
		lowest_index = conf->peak_half_width;

	// I'll get the PEAK_NUMBER maximum peaks.
	for (i = lowest_index; i < N - conf->peak_half_width; i++)
		if (lingot_signal_is_peak(x, i, conf->peak_half_width,
				conf->noise_threshold_nu)) {

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

int lingot_signal_is_peak2(const FLT* signal, int index, short peak_half_width) {
	register unsigned int j;

	for (j = 0; j < peak_half_width; j++) {
		if (signal[index + j] < signal[index + j + 1])
			return 0;
		if (signal[index - j] < signal[index - j - 1])
			return 0;
	}
	return 1;
}

FLT lingot_signal_fft_bin_interpolate_quinn2_tau(FLT x) {
	return (0.25 * log(3 * x * x + 6 * x + 1)
			- 0.102062072615966
					* log(
							(x + 1 - 0.816496580927726)
									/ (x + 1 + 0.816496580927726)));
}

FLT lingot_signal_fft_bin_interpolate_quinn2(const LingotComplex y1,
		const LingotComplex y2, const LingotComplex y3) {
	FLT absy2_2 = y2[0] * y2[0] + y2[1] * y2[1];
	FLT ap = (y3[0] * y2[0] + y3[1] * y2[1]) / absy2_2;
	FLT dp = -ap / (1.0 - ap);
	FLT am = (y1[0] * y2[0] + y1[1] * y2[1]) / absy2_2;
	FLT dm = am / (1.0 - am);
	return (dp + dm) / 2 + lingot_signal_fft_bin_interpolate_quinn2_tau(dp * dp)
			- lingot_signal_fft_bin_interpolate_quinn2_tau(dm * dm);
}

// search the fundamental peak given the spd and its 2nd derivative
int lingot_signal_get_fundamental_peak2(const FLT* signal, const FLT* noise,
		int N, int n_peaks, int lowest_index, short peak_half_width,
		FLT min_snr) {
	register unsigned int i, j, m;
	int p_index[n_peaks];

// at this moment there are no peaks.
	for (i = 0; i < n_peaks; i++)
		p_index[i] = -1;

	if (lowest_index < peak_half_width) {
		lowest_index = peak_half_width;
	}

// I'll get the PEAK_NUMBER maximum peaks with SNR above the required.
	for (i = lowest_index; i < N - peak_half_width; i++)
		if ((signal[i] - noise[i] > min_snr)
				&& lingot_signal_is_peak2(signal, i, peak_half_width)) {

			printf("%d %f\n", (signal[i] - noise[i] > min_snr),
					(signal[i] - noise[i]));
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
				if (signal[p_index[j]] < signal[p_index[m]]) {
					m = j;
				}
			}

			if (p_index[m] == -1) {
				p_index[m] = i; // there is a place
			} else if (signal[i] > signal[p_index[m]]) {
				p_index[m] = i; // if greater
			}
		}

	FLT maximum = 0.0;
	int maximum_index = -1;

// search the maximum peak
	for (i = 0; i < n_peaks; i++)
		if ((p_index[i] != -1) && (signal[p_index[i]] > maximum)) {
			maximum = signal[p_index[i]];
			maximum_index = p_index[i];
		}

	for (i = 0; i < n_peaks; i++) {
		printf("%i ", p_index[i]);
	}
	printf("\n");

	if (maximum_index == -1)
		return N;

//	// all peaks much lower than maximum are deleted.
//	for (i = 0; i < n_peaks; i++)
//		if ((p_index[i] == -1)
//				|| (conf->peak_rejection_relation_nu * signal[p_index[i]]
//						< maximum))
//			p_index[i] = N; // there are available places in the buffer.

// search the lowest maximum index.
	for (m = 0, j = 0; j < n_peaks; j++) {
		if (p_index[j] < p_index[m])
			m = j;
	}

	return p_index[m];
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

// generates a Hamming window of N samples
void lingot_signal_window(int N, FLT* out, window_type_t window_type) {
	register int i;
	switch (window_type) {
	case RECTANGULAR:
		for (i = 0; i < N; i++)
			out[i] = 1.0;
		break;
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

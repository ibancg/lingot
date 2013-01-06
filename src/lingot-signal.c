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

/*
 peak identification functions.
 */

FLT lingot_signal_get_noise_threshold(const LingotConfig* conf, FLT w) {
	//return 0.5*(1.0 - 0.9*w/M_PI);
	return pow(10.0, (conf->noise_threshold_db * (1.0 - 0.9 * w / M_PI)) / 10.0);
	//return conf->noise_threshold_un;
}

//---------------------------------------------------------------------------

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

int lingot_signal_is_peak(const LingotConfig* conf, const FLT* x, int index) {
	register unsigned int j;
	//static   FLT  delta_w_FFT = 2.0*M_PI/conf->FFT_SIZE; // resolution in rads

	// a peak must be greater than noise threshold.
	if (x[index] < conf->noise_threshold_nu)
		return 0;

	for (j = 0; j < conf->peak_half_width; j++) {
		if (x[index + j] < x[index + j + 1])
			return 0;
		if (x[index - j] < x[index - j - 1])
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
		if (lingot_signal_is_peak(conf, x, i)) {

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

void lingot_signal_compute_noise_level(const FLT* spd, int N, int cbuffer_size,
		FLT* noise_level) {

	static int n = 0;
	static int _np2 = 0.0;
	static FLT* cbuffer = NULL;
	static FLT _1pn = 0.0;

	if (cbuffer_size != n) {
		if (cbuffer != NULL ) {
			free(cbuffer);
		}
		n = cbuffer_size;
		cbuffer = malloc(n * sizeof(FLT));
		_np2 = n / 2;
		_1pn = 1.0 / n;
	}

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

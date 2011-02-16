/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2011  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#include <math.h>
#include <stdlib.h>

#include "lingot-fft.h"
#include "lingot-config.h"

/*
 DFT functions.
 */

#ifndef LIB_FFTW

#include "lingot-complex.h"

// phase factor table, for FFT optimization.
LingotComplex* wn = NULL;

// creates the phase factor table.
void lingot_fft_create_phase_factors(LingotConfig* conf) {
	FLT alpha;
	unsigned int i;

	wn = (LingotComplex*) malloc((conf->fft_size >> 1) * sizeof(LingotComplex));

	for (i = 0; i < (conf->fft_size >> 1); i++) {
		alpha = -2.0 * i * M_PI / conf->fft_size;
		wn[i].r = cos(alpha);
		wn[i].i = sin(alpha);
	}
}

void lingot_fft_destroy_phase_factors() {
	if (wn != NULL) {
		free(wn);
	}
}

//------------------------------------------------------------------------

// Fast Fourier Transform.
void _lingot_fft_fft(FLT* in, LingotComplex* out, unsigned long int N,
		unsigned long int offset, unsigned long int d1, unsigned long int step) {
	LingotComplex X1, X2;
	unsigned long int Np2 = (N >> 1); // N/2
	register unsigned long int a, b, c, q;

	if (N == 2) { // butterfly for N = 2;

		X1.r = in[offset];
		X1.i = 0.0;
		X2.r = in[offset + step];
		X2.i = 0.0;

		lingot_complex_add(&X1, &X2, &out[d1]);
		lingot_complex_sub(&X1, &X2, &out[d1 + Np2]);

		return;
	}

	_lingot_fft_fft(in, out, Np2, offset, d1, step << 1);
	_lingot_fft_fft(in, out, Np2, offset + step, d1 + Np2, step << 1);

	for (q = 0, c = 0; q < (N >> 1); q++, c += step) {

		a = q + d1;
		b = a + Np2;

		X1 = out[a];
		lingot_complex_mul(&out[b], &wn[c], &X2);
		lingot_complex_add(&X1, &X2, &out[a]);
		lingot_complex_sub(&X1, &X2, &out[b]);
	}
}

void lingot_fft_fft(FLT* in, LingotComplex* out, unsigned long int N) {
	_lingot_fft_fft(in, out, N, 0, 0, 1);
}

#endif

//------------------------------------------------------------------------


/* Spectral Power Distribution esteem, selectively in frequency, by DFT.
 transforms signal in of N1 samples from frequency wi, with sample
 separation of dw rads, storing the result on buffer out with N2 samples. */
void lingot_fft_spd(FLT* in, int N1, FLT wi, FLT dw, FLT* out, int N2) {
	FLT Xr, Xi;
	FLT wn;
	FLT N1_2 = N1 * N1;
	register int i, n;

	for (i = 0; i < N2; i++) {

		Xr = 0.0;
		Xi = 0.0;

		for (n = 0; n < N1; n++) { // O(n1*n2)  :(

			wn = (wi + dw * i) * n;
			Xr = Xr + cos(wn) * in[n];
			Xi = Xi - sin(wn) * in[n];
		}

		out[i] = (Xr * Xr + Xi * Xi) / N1_2; // normalized squared module.
	}
}

//------------------------------------------------------------------------

/*
 Evaluates 1st and 2nd derivatives from SPD at frequency w.
 */
void lingot_fft_spd_diffs(FLT* in, int N, FLT w, FLT* out_d1, FLT* out_d2) {
	FLT x_cos_wn;
	FLT x_sin_wn;
	register int n;

	FLT SUM_x_sin_wn = 0.0;
	FLT SUM_x_cos_wn = 0.0;
	FLT SUM_x_n_sin_wn = 0.0;
	FLT SUM_x_n_cos_wn = 0.0;
	FLT SUM_x_n2_sin_wn = 0.0;
	FLT SUM_x_n2_cos_wn = 0.0;

	for (n = 0; n < N; n++) {

		x_cos_wn = in[n] * cos(w * n);
		x_sin_wn = in[n] * sin(w * n);

		SUM_x_sin_wn += x_sin_wn;
		SUM_x_cos_wn += x_cos_wn;
		SUM_x_n_sin_wn += x_sin_wn * n;
		SUM_x_n_cos_wn += x_cos_wn * n;
		SUM_x_n2_sin_wn += x_sin_wn * n * n;
		SUM_x_n2_cos_wn += x_cos_wn * n * n;
	}

	FLT N_2 = N * N;
	*out_d1 = 2.0 * (SUM_x_sin_wn * SUM_x_n_cos_wn - SUM_x_cos_wn
			* SUM_x_n_sin_wn) / N_2;
	*out_d2 = 2.0 * (SUM_x_n_cos_wn * SUM_x_n_cos_wn - SUM_x_sin_wn
			* SUM_x_n2_sin_wn + SUM_x_n_sin_wn * SUM_x_n_sin_wn - SUM_x_cos_wn
			* SUM_x_n2_cos_wn) / N_2;
}

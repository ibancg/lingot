/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña.
 * Copyright (C) 2004-2008  Jairo Chapela Martínez.

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
#include <string.h>

#include "lingot-fft.h"
#include "lingot-config.h"

#ifndef LIBFFTW
#include "lingot-complex.h"
#endif

/*
 DTFT functions.
 */

LingotFFTPlan* lingot_fft_plan_create(FLT* in, int n) {

	LingotFFTPlan* result = malloc(sizeof(LingotFFTPlan));
	result->n = n;
	result->in = in;

#ifdef LIBFFTW
	result->fft_out = fftw_malloc(n * sizeof(fftw_complex));
	memset(result->fft_out, 0, n * sizeof(fftw_complex));
	result->fftwplan = fftw_plan_dft_r2c_1d(n, in, result->fft_out,
			FFTW_ESTIMATE);
#else
	FLT alpha;
	unsigned int i;

	// twiddle factors
	result->wn = (LingotComplex*) malloc((n >> 1) * sizeof(LingotComplex));

	for (i = 0; i < (n >> 1); i++) {
		alpha = -2.0 * i * M_PI / n;
		result->wn[i][0] = cos(alpha);
		result->wn[i][1] = sin(alpha);
	}
	result->fft_out = malloc(n * sizeof(LingotComplex)); // complex signal in freq domain.
	memset(result->fft_out, 0, n * sizeof(LingotComplex));
#endif

	return result;
}

void lingot_fft_plan_destroy(LingotFFTPlan* plan) {

#ifdef LIBFFTW
	fftw_destroy_plan(plan->fftwplan);
	fftw_free(plan->fft_out);
#else
	free(plan->fft_out);
	free(plan->wn);
#endif

	free(plan);
}

#ifndef LIBFFTW

void _lingot_fft_fft(FLT* in, LingotComplex* out, LingotComplex* wn, unsigned long int N,
		unsigned long int offset, unsigned long int d1, unsigned long int step) {
	LingotComplex X1, X2;
	unsigned long int Np2 = (N >> 1); // N/2
	register unsigned long int a, b, c, q;

	if (N == 2) { // butterfly for N = 2;

		X1[0] = in[offset];
		X1[1] = 0.0;
		X2[0] = in[offset + step];
		X2[1] = 0.0;

		lingot_complex_add(X1, X2, out[d1]);
		lingot_complex_sub(X1, X2, out[d1 + Np2]);

		return;
	}

	_lingot_fft_fft(in, out, wn, Np2, offset, d1, step << 1);
	_lingot_fft_fft(in, out, wn, Np2, offset + step, d1 + Np2, step << 1);

	for (q = 0, c = 0; q < (N >> 1); q++, c += step) {

		a = q + d1;
		b = a + Np2;

		X1[0] = out[a][0];
		X1[1] = out[a][1];
		lingot_complex_mul(out[b], wn[c], X2);
		lingot_complex_add(X1, X2, out[a]);
		lingot_complex_sub(X1, X2, out[b]);
	}
}

void lingot_fft_fft(LingotFFTPlan* plan) {
	_lingot_fft_fft(plan->in, plan->fft_out, plan->wn, plan->n, 0, 0, 1);
}

#endif

void lingot_fft_compute_dft_and_spd(LingotFFTPlan* plan, FLT* out, int n_out) {

	int i;
	double _1_N2 = 1.0 / (plan->n * plan->n);

# ifdef LIBFFTW
	// transformation.
	fftw_execute(plan->fftwplan);
# else
	// transformation.
	lingot_fft_fft(plan);
#endif

	// esteem of SPD from FFT. (normalized squared module)
	for (i = 0; i < n_out; i++) {
		out[i] = (plan->fft_out[i][0] * plan->fft_out[i][0]
				+ plan->fft_out[i][1] * plan->fft_out[i][1]) * _1_N2;
	}
}

/* Spectral Power Distribution esteem, selectively in frequency, by DFT.
 transforms signal in of N1 samples from frequency wi, with sample
 separation of dw rads, storing the result on buffer out with N2 samples. */
void lingot_fft_spd_eval(FLT* in, int N1, FLT wi, FLT dw, FLT* out, int N2) {
	FLT Xr, Xi;
	FLT wn;
	const FLT N1_2 = N1 * N1;
	int i, n;

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

void lingot_fft_spd_diffs_eval(const FLT* in, int N, FLT w, FLT* out_d0,
		FLT* out_d1, FLT* out_d2) {
	FLT x_cos_wn;
	FLT x_sin_wn;
	const FLT N2 = N * N;

	int n;

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
	*out_d0 = (SUM_x_cos_wn * SUM_x_cos_wn + SUM_x_sin_wn * SUM_x_sin_wn) / N2;
	*out_d1 = 2.0
			* (SUM_x_sin_wn * SUM_x_n_cos_wn - SUM_x_cos_wn * SUM_x_n_sin_wn)
			/ N_2;
	*out_d2 = 2.0
			* (SUM_x_n_cos_wn * SUM_x_n_cos_wn - SUM_x_sin_wn * SUM_x_n2_sin_wn
					+ SUM_x_n_sin_wn * SUM_x_n_sin_wn
					- SUM_x_cos_wn * SUM_x_n2_cos_wn) / N_2;
}

/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2020  Iban Cereijo.
 * Copyright (C) 2004-2008  Jairo Chapela.

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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>

#include "lingot-fft.h"
#include "lingot-config.h"

/*
 DTFT functions.
 */

void lingot_fft_plan_create(lingot_fft_plan_t* result, LINGOT_FLT* in, unsigned int n) {

    result->n = n;
    result->in = in;

#ifdef LIBFFTW
    result->fft_out = fftw_malloc(n * sizeof(fftw_complex));
    memset(result->fft_out, 0, n * sizeof(fftw_complex));
    result->fftwplan = fftw_plan_dft_r2c_1d((int) n, in, result->fft_out, FFTW_ESTIMATE);
#else

    // twiddle factors
    result->wn = (LINGOT_FLT complex*) malloc((n >> 1) * sizeof(LINGOT_FLT complex));

    unsigned int i;
    for (i = 0; i < (n >> 1); i++) {
        result->wn[i] = cexp(-2.0 * i * I * M_PI / n);
    }
    result->fft_out = malloc(n * sizeof(LINGOT_FLT complex)); // complex signal in freq domain.
    memset(result->fft_out, 0, n * sizeof(LINGOT_FLT complex));
#endif

}

void lingot_fft_plan_destroy(lingot_fft_plan_t* plan) {

#ifdef LIBFFTW
    fftw_destroy_plan(plan->fftwplan);
    fftw_free(plan->fft_out);
#else
    free(plan->fft_out);
    free(plan->wn);
    plan->fft_out = NULL;
    plan->wn = NULL;
#endif
}

#ifndef LIBFFTW

void _lingot_fft_fft(LINGOT_FLT* in, LINGOT_FLT complex* out, LINGOT_FLT complex* wn, unsigned long int N,
                     unsigned long int offset, unsigned long int d1, unsigned long int step) {
    LINGOT_FLT complex X1, X2;
    unsigned long int Np2 = (N >> 1); // N/2
    unsigned long int a, b, c, q;

    if (N == 2) { // butterfly for N = 2;

        X1 = in[offset];
        X2 = in[offset + step];

        out[d1]         = X1 + X2;
        out[d1 + Np2]   = X1 - X2;
        return;
    }

    _lingot_fft_fft(in, out, wn, Np2, offset, d1, step << 1);
    _lingot_fft_fft(in, out, wn, Np2, offset + step, d1 + Np2, step << 1);

    for (q = 0, c = 0; q < (N >> 1); q++, c += step) {

        a = q + d1;
        b = a + Np2;

        X1 = out[a];
        X2 = out[b] * wn[c];
        out[a] = X1 + X2;
        out[b] = X1 - X2;
    }
}

void lingot_fft_fft(lingot_fft_plan_t* plan) {
    _lingot_fft_fft(plan->in, plan->fft_out, plan->wn, plan->n, 0, 0, 1);
}

#endif

void lingot_fft_compute_dft_and_spd(lingot_fft_plan_t* plan, LINGOT_FLT* out, unsigned int n_out) {

    unsigned int i;
    double _1_N2 = 1.0 / (plan->n * plan->n);

# ifdef LIBFFTW
    // transformation.
    fftw_execute(plan->fftwplan);
# else
    // transformation.
    lingot_fft_fft(plan);
#endif

    typedef LINGOT_FLT lingot_complex_t[2];
    lingot_complex_t* _out = (lingot_complex_t*) plan->fft_out;

    // estimation of SPD from FFT. (normalized squared module)
    for (i = 0; i < n_out; i++) {
        out[i] = (_out[i][0] * _out[i][0] + _out[i][1] * _out[i][1]) * _1_N2;
    }
}

/* Spectral Power Distribution estimation, selectively in frequency, by DFT.
 transforms signal in of N1 samples from frequency wi, with sample
 separation of dw rads, storing the result on buffer out with N2 samples. */
void lingot_fft_spd_eval(LINGOT_FLT* in, unsigned int N1, LINGOT_FLT wi, LINGOT_FLT dw, LINGOT_FLT* out, unsigned int N2) {
    LINGOT_FLT Xr, Xi;
    LINGOT_FLT wn;
    const LINGOT_FLT N1_2 = N1 * N1;
    unsigned int i;
    unsigned int n;

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

void lingot_fft_spd_diffs_eval(const LINGOT_FLT* in, unsigned int N, LINGOT_FLT w, LINGOT_FLT* out_d0,
                               LINGOT_FLT* out_d1, LINGOT_FLT* out_d2) {
    LINGOT_FLT x_cos_wn;
    LINGOT_FLT x_sin_wn;
    const LINGOT_FLT N2 = N * N;

    unsigned int n;

    LINGOT_FLT SUM_x_sin_wn = 0.0;
    LINGOT_FLT SUM_x_cos_wn = 0.0;
    LINGOT_FLT SUM_x_n_sin_wn = 0.0;
    LINGOT_FLT SUM_x_n_cos_wn = 0.0;
    LINGOT_FLT SUM_x_n2_sin_wn = 0.0;
    LINGOT_FLT SUM_x_n2_cos_wn = 0.0;

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

    LINGOT_FLT N_2 = N * N;
    *out_d0 = (SUM_x_cos_wn * SUM_x_cos_wn + SUM_x_sin_wn * SUM_x_sin_wn) / N2;
    *out_d1 = 2.0
            * (SUM_x_sin_wn * SUM_x_n_cos_wn - SUM_x_cos_wn * SUM_x_n_sin_wn)
            / N_2;
    *out_d2 = 2.0
            * (SUM_x_n_cos_wn * SUM_x_n_cos_wn - SUM_x_sin_wn * SUM_x_n2_sin_wn
               + SUM_x_n_sin_wn * SUM_x_n_sin_wn
               - SUM_x_cos_wn * SUM_x_n2_cos_wn) / N_2;
}

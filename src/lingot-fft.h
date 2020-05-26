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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LINGOT_FFT_H
#define LINGOT_FFT_H

/*
 Fourier transforms.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef LIBFFTW
# include <fftw3.h>
#endif

#include "lingot-defs.h"
#include <complex.h>

typedef struct {

    unsigned int n;
    LINGOT_FLT* in;

#ifdef LIBFFTW
    fftw_plan fftwplan;
#else
    // phase factor table, for FFT optimization.
    LINGOT_FLT complex* wn;
#endif
    LINGOT_FLT complex* fft_out; // complex signal in freq.
} lingot_fft_plan_t;

void lingot_fft_plan_create(lingot_fft_plan_t*, LINGOT_FLT* in, unsigned int n);
void lingot_fft_plan_destroy(lingot_fft_plan_t*);

// Full Spectral Power Distribution (SPD) esteem.
void lingot_fft_compute_dft_and_spd(lingot_fft_plan_t*, LINGOT_FLT* out, unsigned int n_out);

// Spectral Power Distribution (SPD) evaluation at a given frequency.
void lingot_fft_spd_eval(LINGOT_FLT* in, unsigned int N1, LINGOT_FLT wi, LINGOT_FLT dw, LINGOT_FLT* out, unsigned int N2);

// Evaluates first and second SPD derivatives at frequency w.
void lingot_fft_spd_diffs_eval(const LINGOT_FLT* in, unsigned int N, LINGOT_FLT w, LINGOT_FLT* out_d0,
                               LINGOT_FLT* out_d1, LINGOT_FLT* out_d2);

#endif

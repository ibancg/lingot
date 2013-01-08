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

#ifndef _LINGOT_FFT_H_
#define _LINGOT_FFT_H_

/*
 Fourier transforms.
 */

#include "lingot-defs.h"

#ifdef LIBFFTW
# include <fftw3.h>
#endif

# include "lingot-complex.h"

typedef struct _LingotFFTPlan LingotFFTPlan;

struct _LingotFFTPlan {

	int n;
	FLT* in;

#ifdef LIBFFTW
	fftw_plan fftwplan;
#else
// phase factor table, for FFT optimization.
	LingotComplex* wn;
#endif
	LingotComplex* fft_out; // complex signal in freq.
};

LingotFFTPlan* lingot_fft_plan_create(FLT* in, int n);
void lingot_fft_plan_destroy(LingotFFTPlan*);

// Full Spectral Power Distribution (SPD) esteem.
void lingot_fft_compute_dft_and_spd(LingotFFTPlan*, FLT* out, int n_out);

// Spectral Power Distribution (SPD) evaluation at a given frequency.
void lingot_fft_spd_eval(FLT* in, int N1, FLT wi, FLT dw, FLT* out, int N2);

// Evaluates first and second SPD derivatives at frequency w.
void lingot_fft_spd_diffs_eval(FLT* in, int N, FLT w, FLT* out_d1, FLT* out_d2);

#endif

//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004, 2005  Ibán Cereijo Graña, Jairo Chapela Martínez.

  This file is part of lingot.

  lingot is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  lingot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with lingot; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _FFT_H_
#define _FFT_H_

/*
  Fourier transforms.
*/

#include "defs.h"

#ifndef LIB_FFTW

#include "complex.h"

class CPX;
class Config;

void createWN(Config* conf);
void destroyWN();

// Fast Fourier Transform implementation.
void FFT(FLT* in, CPX* out, unsigned long int N, // principal parameters.
	 unsigned long int offset = 0,     // hidden parameters.
	 unsigned long int d1 = 0,
	 unsigned long int step = 1);
#endif

// Spectral Power Distribution (SPD) esteem, selectively in frequency.
void SPD(FLT* in, int N1, FLT wi, FLT dw, FLT* out, int N2);

// first and second SPD derivatives at frequency w.
void SPD_diffs(FLT* in, int N, FLT w, FLT* out_d1, FLT* out_d2);

#endif

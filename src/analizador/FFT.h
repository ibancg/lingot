//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004   Ibán Cereijo Graña, Jairo Chapela Martínez.

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
 Transformadas de Fourier.
*/

#include "defs.h"

#ifndef LIB_FFTW

#include "complex.h"

class CPX;
class Config;

typedef unsigned long int ULI;

void CreaWN(Config* conf);
void DestruyeWN();

// transformada rápida de Fourier.
void FFT(FLT* x, CPX* X, ULI N, ULI i = 0, ULI d1 = 0, ULI d2 = 1);
#endif

// DEP selectiva en frecuencia.
void DEP(FLT* buffer, int N1, FLT wi, FLT dw, FLT* X, int N2);

/* Evalúa la derivada y la derivada segunda de la DEP de buffer en la frecuencia w. */
void diffs_DEP(FLT* buffer, int N, FLT w, FLT &d1, FLT &d2);

#endif

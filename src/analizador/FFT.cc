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

#include <math.h>
#include <stdlib.h>

#include "FFT.h"
#include "config.h"

/*
  Funciones para cálculo de FFT y otros parámetros relacionados
  con la DFT.
 */

#ifndef LIB_FFTW

#include "complex.h"

// tabla con factores de fase. Para acelerar un poco la FFT.
CPX* WN;

// Crea la tabla de los factores de fase.
void CreaWN(Config* conf)
{
  FLT alpha;
  WN = new CPX[conf->FFT_SIZE >> 1];

  for (unsigned int i = 0; i < (conf->FFT_SIZE >> 1); i++) {
    alpha = -2.0*i*M_PI/conf->FFT_SIZE;
    WN[i].r = cos(alpha);
    WN[i].i = sin(alpha);
  }
}

void DestruyeWN() {
  delete [] WN;
}

// transformada rápida de Fourier.
void FFT(FLT* x, CPX* X, ULI N, ULI offset, ULI d1, ULI step)
{
  CPX           X1, X2;
  ULI           Np2 = (N >> 1); // N/2
  register ULI  a, b, c, q;

  if (N == 2) { // Mariposa para N = 2;
    
    X1.r = x[offset];
    X1.i = 0.0;
    X2.r = x[offset + step];
    X2.i = 0.0;
    X[d1].r       = X1.r + X2.r;
    X[d1].i       = X1.i + X2.i;
    X[d1 + Np2].r = X1.r - X2.r;
    X[d1 + Np2].i = X1.i - X2.i;

    return;
  }

  FFT(x, X, Np2, offset, d1, step << 1);
  FFT(x, X, Np2, offset + step, d1 + Np2, step << 1);
    
  for (q = 0, c = 0; q < (N >> 1); q++, c += step) {

    a = q + d1;
    b = a + Np2;

    X1 = X[a];
    MulCPX(X[b], WN[c], X2);

    X[a].r = X1.r + X2.r;
    X[a].i = X1.i + X2.i;
    X[b].r = X1.r - X2.r;
    X[b].i = X1.i - X2.i;
  }
}

#endif

//------------------------------------------------------------------------


/* DFT. DEP SELECTIVA EN FRECUENCIA. (densidad espectral de potencia)
   transforma el buffer de longitud N1, desde la frecuencia wi, con una se
   paración entre muestras de dw rad, dejándolo en el buffer de reales
   X de longitud N2. */
void DEP(FLT* buffer, int N1, FLT wi, FLT dw, FLT* X, int N2)
{
  FLT           Xr, Xi;
  FLT           wn;
  FLT           N1_2 = N1*N1;
  register int  i, n;

  for (i = 0; i < N2; i++) {
    
    Xr = 0.0; // parte real.
    Xi = 0.0; // parte imaginaria.
    
    for (n = 0; n < N1; n++) {
      
      wn = (wi + dw*i)*n;
      Xr = Xr + cos(wn)*buffer[n];
      Xi = Xi - sin(wn)*buffer[n];
    }

    X[i] = (Xr*Xr + Xi*Xi)/N1_2; // móulo al cuadrado.
  }
}

//------------------------------------------------------------------------

/*
  Evalúa la derivada y la derivada segunda de la DEP de buffer en la frecuencia w.
  Se evalúan las 2 derivadas conjuntamente ya que hay mucho cálculo común.
*/
void diffs_DEP(FLT* buffer, int N, FLT w, FLT &d1, FLT &d2)
{
  FLT   x_cos_wn, x_sin_wn;
  FLT   SUM_x_sin_wn    = 0.0;
  FLT   SUM_x_cos_wn    = 0.0;
  FLT   SUM_x_n_sin_wn  = 0.0;
  FLT   SUM_x_n_cos_wn  = 0.0;
  FLT   SUM_x_n2_sin_wn = 0.0;
  FLT   SUM_x_n2_cos_wn = 0.0;
  
  for (register int n = 0; n < N; n++) {
    
    x_cos_wn = buffer[n]*cos(w*n);
    x_sin_wn = buffer[n]*sin(w*n);

    SUM_x_sin_wn    += x_sin_wn;
    SUM_x_cos_wn    += x_cos_wn;
    SUM_x_n_sin_wn  += x_sin_wn*n;
    SUM_x_n_cos_wn  += x_cos_wn*n;
    SUM_x_n2_sin_wn += x_sin_wn*n*n;
    SUM_x_n2_cos_wn += x_cos_wn*n*n;
  }

  FLT N_2 = N*N;
  d1 = 2.0*(SUM_x_sin_wn*SUM_x_n_cos_wn - SUM_x_cos_wn*SUM_x_n_sin_wn)/N_2;
  d2 = 2.0*(SUM_x_n_cos_wn*SUM_x_n_cos_wn - SUM_x_sin_wn*SUM_x_n2_sin_wn +
	    SUM_x_n_sin_wn*SUM_x_n_sin_wn - SUM_x_cos_wn*SUM_x_n2_cos_wn)/N_2;
}

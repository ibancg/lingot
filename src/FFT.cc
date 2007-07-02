//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.

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
  DFT functions.
*/

#ifndef LIB_FFTW

#include "complex.h"

// phase factor table, for FFT optimization.
CPX* WN;

// creates the phase factor table.
void createWN(Config* conf)
{
  FLT alpha;
  WN = new CPX[conf->FFT_SIZE >> 1];

  for (unsigned int i = 0; i < (conf->FFT_SIZE >> 1); i++) {
    alpha = -2.0*i*M_PI/conf->FFT_SIZE;
    WN[i].r = cos(alpha);
    WN[i].i = sin(alpha);
  }
}

void destroyWN() {
  delete [] WN;
}

//------------------------------------------------------------------------

// Fast Fourier Transform.
void FFT(FLT* in, CPX* out, unsigned long int N,
	 unsigned long int offset,
	 unsigned long int d1,
	 unsigned long int step)
{
  CPX                        X1, X2;
  unsigned long int          Np2 = (N >> 1); // N/2
  register unsigned long int a, b, c, q;

  if (N == 2) { // butterfly for N = 2;
    
    X1.r = in[offset];
    X1.i = 0.0;
    X2.r = in[offset + step];
    X2.i = 0.0;

    addCPX(X1, X2, &out[d1]);
    subCPX(X1, X2, &out[d1 + Np2]);

    return;
  }

  FFT(in, out, Np2, offset, d1, step << 1);
  FFT(in, out, Np2, offset + step, d1 + Np2, step << 1);
    
  for (q = 0, c = 0; q < (N >> 1); q++, c += step) {
    
    a = q + d1;
    b = a + Np2;
    
    X1 = out[a];
    mulCPX(out[b], WN[c], &X2);
    addCPX(X1, X2, &out[a]);
    subCPX(X1, X2, &out[b]);
  }
}

#endif

//------------------------------------------------------------------------


/* Spectral Power Distribution esteem, selectively in frequency, by DFT.
   transforms signal in of N1 samples from frequency wi, with sample
   separation of dw rads, storing the result on buffer out with N2 samples. */
void SPD(FLT* in, int N1, FLT wi, FLT dw, FLT* out, int N2)
{
  FLT           Xr, Xi;
  FLT           wn;
  FLT           N1_2 = N1*N1;
  register int  i, n;

  for (i = 0; i < N2; i++) {
    
    Xr = 0.0;
    Xi = 0.0;
    
    for (n = 0; n < N1; n++) { // O(n1*n2)  :(
      
      wn = (wi + dw*i)*n;
      Xr = Xr + cos(wn)*in[n];
      Xi = Xi - sin(wn)*in[n];
    }

    out[i] = (Xr*Xr + Xi*Xi)/N1_2; // normalized squared module.
  }
}

//------------------------------------------------------------------------

/*
  Evaluates 1st and 2nd derivatives from SPD at frequency w.
*/
void SPD_diffs(FLT* in, int N, FLT w, FLT* out_d1, FLT* out_d2)
{
  FLT   x_cos_wn, x_sin_wn;
  FLT   SUM_x_sin_wn    = 0.0;
  FLT   SUM_x_cos_wn    = 0.0;
  FLT   SUM_x_n_sin_wn  = 0.0;
  FLT   SUM_x_n_cos_wn  = 0.0;
  FLT   SUM_x_n2_sin_wn = 0.0;
  FLT   SUM_x_n2_cos_wn = 0.0;
  
  for (register int n = 0; n < N; n++) {
    
    x_cos_wn = in[n]*cos(w*n);
    x_sin_wn = in[n]*sin(w*n);

    SUM_x_sin_wn    += x_sin_wn;
    SUM_x_cos_wn    += x_cos_wn;
    SUM_x_n_sin_wn  += x_sin_wn*n;
    SUM_x_n_cos_wn  += x_cos_wn*n;
    SUM_x_n2_sin_wn += x_sin_wn*n*n;
    SUM_x_n2_cos_wn += x_cos_wn*n*n;
  }

  FLT N_2 = N*N;
  *out_d1 = 2.0*(SUM_x_sin_wn*SUM_x_n_cos_wn - SUM_x_cos_wn*SUM_x_n_sin_wn)/N_2;
  *out_d2 = 2.0*(SUM_x_n_cos_wn*SUM_x_n_cos_wn - SUM_x_sin_wn*SUM_x_n2_sin_wn +
		 SUM_x_n_sin_wn*SUM_x_n_sin_wn - SUM_x_cos_wn*SUM_x_n2_cos_wn)/N_2;
}

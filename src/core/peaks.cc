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

#include "config.h"
#include "core.h"

/*
  peak identification functions.
 */

FLT Core::noise_threshold(FLT w)
{
  //  return 0.5*(1.0 - index/conf.FFT_SIZE);
  return conf.NOISE_THRESHOLD_UN;
}

//---------------------------------------------------------------------------

void Core::max(FLT *x, int N, int* Mi)
{
  register int i;
  FLT          M;
  
  M  = -1.0;
  *Mi = -1;

  for (i = 0; i < N; i++) {
    
    if (x[i] > M) {
      M  = x[i];
      *Mi = i;
    }
  }
}

//---------------------------------------------------------------------------

inline bool Core::peak(FLT* x, int index)
{
  //static   FLT  delta_w_FFT = 2.0*M_PI/conf.FFT_SIZE; // resolution in rads

  // a peak must be greater than noise threshold.
  if (x[index] < conf.NOISE_THRESHOLD_UN) return false;

  for (register unsigned int j = 0; j < conf.PEAK_ORDER; j++) {
    if (x[index + j] < x[index + j + 1]) return false;
    if (x[index - j] < x[index - j - 1]) return false;
  }
  return true;
}

//---------------------------------------------------------------------------

int Core::fundamentalPeak(FLT *x, FLT* y, int N)
{
  register unsigned int i, j, m;
  int                   p_index[conf.PEAK_NUMBER];
  
  // at this moment there is no peaks.
  for (i = 0; i < conf.PEAK_NUMBER; i++) p_index[i] = -1;

  for (i = conf.PEAK_ORDER; i < N - conf.PEAK_ORDER; i++)
    if (peak(y, i)) {

      // search a place in the maximums buffer, if it doesn't exists, the
      // lower maximum is candidate to be replaced.
      m = 0; // first candidate.
      for (j = 0; j < conf.PEAK_NUMBER; j++) {
	if (p_index[j] == -1) {
	  m = j; // place.
	  break;
	}

	if (x[p_index[j]] < x[p_index[m]]) m = j; // search the lowest.
      }

      if (p_index[m] == -1) p_index[m] = i;
      else if (x[i] > x[p_index[m]]) p_index[m] = i; 
  }

  FLT M = 0.0;
  for (i = 0; i < conf.PEAK_NUMBER; i++) 
    if ((p_index[i] != -1) && (y[p_index[i]] > M)) M = y[p_index[i]];
  for (i = 0; i < conf.PEAK_NUMBER; i++) 
    if ((p_index[i] == -1) || 
	(conf.PEAK_REJECTION_RELATION_UN*y[p_index[i]] < M)) 
      p_index[i] = N; // there are available places in the buffer.

  for (m = 0, j = 0; j < conf.PEAK_NUMBER; j++)
    if (p_index[j] < p_index[m]) m = j; // search the lowest index.

  return p_index[m];
}


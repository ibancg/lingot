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
#include "analizador.h"

/*
  Funciones varias para identificar picos.
 */

/* devuelve el nivel considerado ruido a la frecuencia dada.
   La cota puede depender de la frecuencia, debemos ser más permisivos
   para frecuencias altas, ya que su salida no es tan potente. */
FLT Analizador::ruido(FLT w)
{
  return conf.NOISE_THRESHOLD_UN; // en este caso constante.
}

//---------------------------------------------------------------------------

// devuelve el máximo (su índice) de un buffer de N muestras.
void Analizador::maximo(FLT *x, int N, int &Mi)
{
  register int i;
  FLT          M;
  
  M  = -1.0;
  Mi = -1;

  for (i = 0; i < N; i++) {
    
    if (x[i] > M) {
      M  = x[i];
      Mi = i;
    }
  }
}

//---------------------------------------------------------------------------

inline bool Analizador::esPico(FLT* x, int index_muestra)
{
  //static   FLT  delta_w_FFT = 2.0*M_PI/conf.FFT_SIZE; // resolución en radianes.  

  // un pico debe estar por encima del nivel de ruido.
  if (x[index_muestra] < conf.NOISE_THRESHOLD_UN) return false;

  //if (x[index_muestra] < 0.5*(1.0 - index_muestra/conf.FFT_SIZE)) return false;
  for (register unsigned int j = 0; j < conf.PEAK_ORDER; j++) {
    if (x[index_muestra + j] < x[index_muestra + j + 1]) return false;
    if (x[index_muestra - j] < x[index_muestra - j - 1]) return false;
  }
  return true;
}

//---------------------------------------------------------------------------

/* devuelve el pico fundamental (indice) de un buffer de N muestras. */
int Analizador::picoFundamental(FLT *x, FLT* y, int N)
{
  register unsigned int i, j, m;
  int                   p_index[conf.PEAK_NUMBER];
  
  // de momento todos están en el índice -1, no hay picos (convenio).
  for (i = 0; i < conf.PEAK_NUMBER; i++) p_index[i] = -1;

  for (i = conf.PEAK_ORDER; i < N - conf.PEAK_ORDER; i++)  // recorro el buffer.
    if (esPico(y, i)) {

      // busco un hueco en el buffer de máximos, si no lo hay me quedo con el menor
      // como candidato a ser sustituido.
      m = 0; // primer candidato.
      for (j = 0; j < conf.PEAK_NUMBER; j++) {
	if (p_index[j] == -1) {
	  m = j; // hueco.
	  break; // ya no compruebo mas.
	}

	if (x[p_index[j]] < x[p_index[m]]) m = j; // busca el menor.
      }

      if (p_index[m] == -1) p_index[m] = i;
      else if (x[i] > x[p_index[m]]) p_index[m] = i; 
  }

  FLT M = 0.0;
  for (i = 0; i < conf.PEAK_NUMBER; i++) if ((p_index[i] != -1) && (y[p_index[i]] > M)) M = y[p_index[i]];
  for (i = 0; i < conf.PEAK_NUMBER; i++) if ((p_index[i] == -1) || (conf.PEAK_REJECTION_RELATION_UN*y[p_index[i]] < M)) p_index[i] = N; // quedan huecos.

  for (m = 0, j = 0; j < conf.PEAK_NUMBER; j++)
    if (p_index[j] < p_index[m]) m = j; // busca el menor índice.

  return p_index[m];
}


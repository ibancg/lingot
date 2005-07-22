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

#include <memory.h>
#include "filtro.h"

#define max(a,b) (((a)<(b))?(b):(a))

Filtro::Filtro()
{
  N = 0;
  a = b = s = NULL;
}

// se pasan el ORDEN de cada polinomio y los coeficientes, con estado opcional.
Filtro::Filtro(unsigned int Na, unsigned int Nb, FLT* a, FLT* b, FLT* s)
{
  Filtro::N = max(Na, Nb);

  Filtro::a = new FLT[N + 1];
  Filtro::b = new FLT[N + 1];
  Filtro::s = new FLT[N + 1];

  // inicialización.
  for (unsigned int i = 0; i < N + 1; i++)
    Filtro::a[i] = Filtro::b[i] = Filtro::s[i] = 0.0;

  memcpy(Filtro::a, a, (Na + 1)*sizeof(FLT));
  memcpy(Filtro::b, b, (Nb + 1)*sizeof(FLT));
  if (s) memcpy(Filtro::s, s, (N + 1)*sizeof(FLT));

  for (unsigned int i = 0; i < N + 1; i++) {
    Filtro::a[i] /= a[0]; // hacemos el polinomio mónico.
    Filtro::b[i] /= a[0];
  }

}

// permite cambiar coeficientes del filtro. No permite cambiar orden polinomios.
void Filtro::Actualizar(unsigned int Na, unsigned int Nb, FLT* a, FLT* b)
{
  memcpy(Filtro::a, a, (Na + 1)*sizeof(FLT));
  memcpy(Filtro::b, b, (Nb + 1)*sizeof(FLT));

  for (unsigned int i = 0; i < N + 1; i++) {
    Filtro::a[i] /= a[0]; // hacemos el polinomio mónico.
    Filtro::b[i] /= a[0];
  }
}

// filtro implementado en forma directa II, in y out solapables.
void Filtro::filtrarII(unsigned int n, FLT* in, FLT* out) // filtrar un vector
{
  FLT                    w, y;
  register unsigned int  i;
  register int           j;
  
  for(i = 0; i < n; i++){
    
    w = in[i];
    y = 0.0;
    
    for (j = N - 1; j >= 0; j--) {
      w -= a[j + 1]*s[j];
      y += b[j + 1]*s[j];
      s[j + 1] = s[j];
    }
    
    y += w*b[0];
    s[0] = w;
    
    out[i] = y;
  }
}

FLT Filtro::filtrarII(FLT in) // filtrar una muestra
{
  FLT result;

  filtrarII(1, &in, &result);
  return result;
}

Filtro::~Filtro()
{
  delete [] a;
  delete [] b;
  delete [] s;
}

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

#ifndef __ANALIZADOR_H__
#define __ANALIZADOR_H__

#include <pthread.h>
#include "defs.h"
#include "config.h"

#ifdef LIB_FFTW
# include <fftw.h>
#endif

#ifdef LIBSNDOBJ
#  include <SndObj/AudioDefs.h>
#else
#  include "audio.h"
#endif

class CPX;
class Filtro;

class Analizador {

public:

  Config        conf;       // estructura de configuración
  bool          status;
  Config        nueva_conf; // configuracion pendiente.
  bool          hay_nueva_conf;

  // pthread-related  member variables
  pthread_attr_t  attrib;
  pthread_t       thread;

protected:

  FLT           frecuencia; // frecuencia fundamental analógica calculada.
  FLT           X[256];     // tamaño de visualización fijo, indep del tamaño de la FFT.

private:  

# ifdef LIBSNDOBJ
  SndRTIO*      A;                       // manejador audio.
# else
  audio*        A;                       // manejador audio.
# endif

  TIPO_MUESTRA* buffer_muestras_leidas;  // buffer leído de la tarjeta.
  FLT*          buffer_leido;            // buffer leído de la tarjeta.
  FLT*          buffer_ventana_temporal; // memoria de muestras en tiempo.

  // estimación de la función de densidad espectral de potencia.
  FLT*          dep_fft;
  FLT*          dep_dft;
  FLT*          diff2_dep_fft;

# ifdef LIB_FFTW
  fftw_complex  *fftw_in, *fftw_out; // señal compleja en dominio temporal y frecuencial.
  fftw_plan     fftwplan;
# else
  CPX*          fft_out; // señal compleja en dominio frecuencial.
# endif

  Filtro*       filtro_diezmado; // filtro antialiasing.

  //----------------------------------------------------------------

  // los siguientes métodos están implementados en maximo.cc

  /* devuelve el nivel considerado ruido a la frecuencia dada.
     La cota va a depender de la frecuencia, debemos ser más permisivos
     para frecuencias altas, ya que su salida no es tan potente. */
  FLT           ruido(FLT w);
  
  void          diezmar(FLT* in, FLT* out);
  bool          esPico(FLT* x, int index_muestra);
  
  // devuelve el máximo (su índice) de un buffer de N muestras.
  void          maximo(FLT *x, int N, int &Mi);
  
  // devuelve el pico fundamental (indice) de un buffer de N muestras.
  int           picoFundamental(FLT *x, FLT* y, int N);

public:

  // constructor.
  Analizador();
  // destructor.
  ~Analizador();
  
  // procesa los datos leídos para calcular la frecuencia.
  void procesar();

  void cambiaConfig(Config conf);

  void iniciar();
  void finalizar();
};

void ProcessThread(Analizador*);

#endif //__ANALIZADOR_H__

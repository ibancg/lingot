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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "defs.h"

// Clase de configuración. Determina el funcionamiento del afinador.
// No todos los parámetros son configurables externamente.
class Config {

private:

  char**       tokens;    // tabla de tokens.
  void**       atributos; // tabla de punteros a los atributos.
  char*        formatos;  // formatos para parsear/guardar los tokens de/en el archivo de configuración.

public:
  
  char         AUDIO_DEV[80];
  unsigned int SAMPLE_RATE;
  unsigned int OVERSAMPLING;

  // error de frecuencia del "La" de referencia (en semitonos).
  FLT          ROOT_FREQUENCY_ERROR;

  // frecuencia del "La" de referencia (parámetro derivado).
  FLT          ROOT_FREQUENCY;

  unsigned int FFT_SIZE; // muestras usadas para hacer la FFT del mismo número de muestras.

  FLT          CALCULATION_RATE;   // frecuencia de cálculo.
  FLT          VISUALIZATION_RATE; // frecuencia de visualización.

  // duración en segundos de la ventana temporal.
  FLT          TEMPORAL_WINDOW;  // seg.

  // muestras almacenadas en la ventana temporal (parámetro derivado).
  unsigned int TEMPORAL_BUFFER_SIZE;
  // muestras leidas de la tarjeta de cada vez. (muestras/cálculo, parámetro derivado)
  unsigned int READ_BUFFER_SIZE;

  FLT          NOISE_THRESHOLD;    // dB
  FLT          NOISE_THRESHOLD_UN; // unidades naturales (parámetro derivado)

  // configuración del algoritmo de determinación de la frecuencia fundamental.
  //----------------------------------------------------------------------------

  // número de picos máximos considerados.
  unsigned int PEAK_NUMBER;

  // número de muestras adyacentes para considerar una muestra como pico.
  unsigned int PEAK_ORDER ;

  // si el pico máximo es tantas o más veces mayor que el pico considerado,
  // dicho pico no se tendrá en cuenta como portador de la frecuencia
  // fundamental.
  FLT          PEAK_REJECTION_RELATION;    // dBs
  FLT          PEAK_REJECTION_RELATION_UN; // unidades naturales (parámetro derivado)

  // aproximación por DFT's
  unsigned int DFT_NUMBER;  // numero de DFT's.
  unsigned int DFT_SIZE;  // tamaño en muestras de las DFT's de aproximación.

  // máximo número de iterantes por el algoritmo de Newton-Raphson
  unsigned int MAX_NR_ITER;

  //----------------------------------------------------------------------------

  // valor de reposo de la aguja. (la aguja abarca [-0.5, 0.5])
  FLT          VRP;

  //----------------------------------------------------------------------------

public:
  
  // constructor.
  Config();

  // configuración por defecto.
  void reset();

  // resuelve parámetros internos a partir de parámetros externos.
  void actualizaParametrosInternos();

  void asociaAtributos();
  void guardaArchivoConf(char* archivo);
  void parseaArchivoConf(char* archivo);
};


#endif

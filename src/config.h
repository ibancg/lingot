//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004   Ib�n Cereijo Gra�a, Jairo Chapela Mart�nez.

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

// Clase de configuraci�n. Determina el funcionamiento del afinador.
// No todos los par�metros son configurables externamente.
class Config {

private:

  char**       tokens;    // tabla de tokens.
  void**       atributos; // tabla de punteros a los atributos.
  char*        formatos;  // formatos para parsear/guardar los tokens de/en el archivo de configuraci�n.

public:
  
  char         AUDIO_DEV[80];
  unsigned int SAMPLE_RATE;
  unsigned int OVERSAMPLING;

  // error de frecuencia del "La" de referencia (en semitonos).
  FLT          ROOT_FREQUENCY_ERROR;

  // frecuencia del "La" de referencia (par�metro derivado).
  FLT          ROOT_FREQUENCY;

  unsigned int FFT_SIZE; // muestras usadas para hacer la FFT del mismo n�mero de muestras.

  FLT          CALCULATION_RATE;   // frecuencia de c�lculo.
  FLT          VISUALIZATION_RATE; // frecuencia de visualizaci�n.

  // duraci�n en segundos de la ventana temporal.
  FLT          TEMPORAL_WINDOW;  // seg.

  // muestras almacenadas en la ventana temporal (par�metro derivado).
  unsigned int TEMPORAL_BUFFER_SIZE;
  // muestras leidas de la tarjeta de cada vez. (muestras/c�lculo, par�metro derivado)
  unsigned int READ_BUFFER_SIZE;

  FLT          NOISE_THRESHOLD;    // dB
  FLT          NOISE_THRESHOLD_UN; // unidades naturales (par�metro derivado)

  // configuraci�n del algoritmo de determinaci�n de la frecuencia fundamental.
  //----------------------------------------------------------------------------

  // n�mero de picos m�ximos considerados.
  unsigned int PEAK_NUMBER;

  // n�mero de muestras adyacentes para considerar una muestra como pico.
  unsigned int PEAK_ORDER ;

  // si el pico m�ximo es tantas o m�s veces mayor que el pico considerado,
  // dicho pico no se tendr� en cuenta como portador de la frecuencia
  // fundamental.
  FLT          PEAK_REJECTION_RELATION;    // dBs
  FLT          PEAK_REJECTION_RELATION_UN; // unidades naturales (par�metro derivado)

  // aproximaci�n por DFT's
  unsigned int DFT_NUMBER;  // numero de DFT's.
  unsigned int DFT_SIZE;  // tama�o en muestras de las DFT's de aproximaci�n.

  // m�ximo n�mero de iterantes por el algoritmo de Newton-Raphson
  unsigned int MAX_NR_ITER;

  //----------------------------------------------------------------------------

  // valor de reposo de la aguja. (la aguja abarca [-0.5, 0.5])
  FLT          VRP;

  //----------------------------------------------------------------------------

public:
  
  // constructor.
  Config();

  // configuraci�n por defecto.
  void reset();

  // resuelve par�metros internos a partir de par�metros externos.
  void actualizaParametrosInternos();

  void asociaAtributos();
  void guardaArchivoConf(char* archivo);
  void parseaArchivoConf(char* archivo);
};


#endif

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "config.h"
#include "interfaz.h"

// constructor, valores por defecto.
Config::Config() {
  reset();
}

// valores por defecto.
void Config::reset()
{
  sprintf(AUDIO_DEV,"%s","/dev/dsp");

  SAMPLE_RATE   = 8000;
  OVERSAMPLING  = 5;

  // error de frecuencia del "La" de referencia.
  ROOT_FREQUENCY_ERROR = 0; // Hz

  FFT_SIZE = 256; // muestras usadas para hacer la FFT del mismo número de muestras.

  // duración en segundos de la ventana temporal.
  TEMPORAL_WINDOW = 0.1875;  // seg.

  CALCULATION_RATE   = 20;
  VISUALIZATION_RATE = 25;

  NOISE_THRESHOLD = 10.0;

  // configuración del algoritmo de determinación de la frecuencia fundamental.
  //----------------------------------------------------------------------------

  // número de picos máximos considerados.
  PEAK_NUMBER = 3;

  // número de muestras adyacentes para considerar una muestra como pico.
  PEAK_ORDER = 1;

  // si el pico máximo es tantas o más veces mayor que el pico considerado,
  // dicho pico no se tendrá en cuenta como portador de la frecuencia
  // fundamental.
  PEAK_REJECTION_RELATION = 20;

  // aproximación por DFT's
  DFT_NUMBER = 2;  // numero de DFT's.
  DFT_SIZE     = 15;  // tamaño en muestras de las DFT's de aproximación.

  // máximo número de iterantes por el algoritmo de Newton-Raphson
  MAX_NR_ITER = 10;

  //----------------------------------------------------------------------------

  // valor de reposo de la aguja. Casi al mínimo. (la aguja abarca [-0.5, 0.5])
  VRP = -0.45;

  actualizaParametrosInternos();
}

void Config::actualizaParametrosInternos()
{
  // parámetros derivados.
  ROOT_FREQUENCY             = 440.0*pow(2.0, ROOT_FREQUENCY_ERROR/1200.0);
  TEMPORAL_BUFFER_SIZE       = (unsigned int) (TEMPORAL_WINDOW*SAMPLE_RATE/OVERSAMPLING);
  READ_BUFFER_SIZE           = (unsigned int) (SAMPLE_RATE/(CALCULATION_RATE*OVERSAMPLING));
  PEAK_REJECTION_RELATION_UN = pow(10.0, PEAK_REJECTION_RELATION/10.0);
  NOISE_THRESHOLD_UN         = pow(10.0, NOISE_THRESHOLD/10.0);

  if (TEMPORAL_BUFFER_SIZE < FFT_SIZE) {

#   ifdef QUICK_MESSAGES
    char buff[80];
    TEMPORAL_WINDOW = ((double) FFT_SIZE*OVERSAMPLING)/SAMPLE_RATE;
    sprintf(buff, gettext("Temporal buffer is smaller than FFT size. It has been increased to %0.3f seconds\n"), (float)TEMPORAL_WINDOW);
    quick_message(gettext("tuner warning"), buff);
#   endif
    TEMPORAL_BUFFER_SIZE = FFT_SIZE;
  }
  
}

void Config::asociaAtributos() {

  static char* c_tokens[] = {
    "AUDIO_DEV",
    "SAMPLE_RATE",
    "OVERSAMPLING",
    "ROOT_FREQUENCY_ERROR",
    "FFT_SIZE",
    "TEMPORAL_WINDOW",
    "NOISE_THRESHOLD",
    "CALCULATION_RATE",
    "VISUALIZATION_RATE",
    "PEAK_NUMBER",
    "PEAK_ORDER",
    "PEAK_REJECTION_RELATION",
    "DFT_NUMBER",
    "DFT_SIZE",
    NULL
  };

  // Punteros a los atributos asociados a las palabras clave anteriores.
  static void* c_atributos[] = {
    &AUDIO_DEV,
    &SAMPLE_RATE,
    &OVERSAMPLING,
    &ROOT_FREQUENCY_ERROR,
    &FFT_SIZE,
    &TEMPORAL_WINDOW,
    &NOISE_THRESHOLD,
    &CALCULATION_RATE,
    &VISUALIZATION_RATE,
    &PEAK_NUMBER,
    &PEAK_ORDER,
    &PEAK_REJECTION_RELATION,
    &DFT_NUMBER,
    &DFT_SIZE
  };
  
  // No hago copias.
  tokens    = c_tokens;
  atributos = c_atributos;
  formatos  = "sddfdffffddfdd"; // Formatos de impresión/escaneado de los atributos anteriores.
}

void Config::guardaArchivoConf(char* archivo) {

  FILE* fp;

  asociaAtributos();

  if ((fp = fopen(archivo, "w")) == NULL) {
    char buff[100];
    sprintf(buff, "error saving config file %s ", archivo);
    perror(buff);
    return;
  }

  fprintf(fp, "# Config file automatically created by lingot\n\n");

  for (int i = 0; tokens[i]; i++) switch (formatos[i]) {
  case 's': fprintf(fp, "%s = %s\n", tokens[i], (char*) atributos[i]); break;
  case 'd': fprintf(fp, "%s = %d\n", tokens[i], *((unsigned int*) atributos[i])); break;
  case 'f': fprintf(fp, "%s = %0.3f\n", tokens[i], *((FLT*) atributos[i])); break;
  }

  fclose(fp);
}


void Config::parseaArchivoConf( char* archivo )
{
  //  printf("Config::parseaArchivoConf(this = %p)\n", this);

  FILE* fp;

  asociaAtributos();

# define MAX_LINE_SIZE 100

  char s1[MAX_LINE_SIZE];

  if ((fp = fopen(archivo, "r")) == NULL) {
    sprintf(s1, "error opening config file %s, assuming default values ", archivo);
    perror(s1);
    return;
  }

  int linea = 0;

  do {
    
    linea++;

    if (!fgets(s1, MAX_LINE_SIZE, fp)) break;;

    //    printf("linea %d: %s\n", linea, s1);
    
    if (s1[0] == '#') continue;

    //    char* p2 = s1; // puntero sobre la línea leída.
    char* s2;      // contendrá los tokens dentro de la línea.

    // cojo la cadena del atributo.
    /*do {
      s2 = strsep(&p2, " \t=\n");
      } while ((s2) && (strlen(s2) == 0));*/
    s2 = strtok(s1, " \t=\n");

    if (!s2) continue; // si no encuentra atributo continúa (línea en blanco).

    //    printf("atributo: >>%s<<\n", s2);
    int i;
    for (i = 0; Config::tokens[i]; i++) if (!strcmp(s2, Config::tokens[i])) {
      //      printf("encontrado token %s (%i)\n", tokens[i], i);
      break;
    }

    if (!Config::tokens[i]) {
      fprintf(stderr, "warning: parse error at line %i: unknown keyword %s\n", linea, s2);
      continue;
    }

    // cojo el valor del atributo.
    /*do {
      s2 = strsep(&p2, " \t=\n");
      } while ((s2) && (strlen(s2) == 0));*/
    s2 = strtok(NULL, " \t=\n");
    
    if (!s2) {
      fprintf(stderr, "warning: parse error at line %i: value expected\n", linea);
      continue;
    }

    // asigno el valor al atributo.
    switch (Config::formatos[i]) {
    case 's' : 
      /*      char buff[MAX_LINE_SIZE];
	      sscanf(s2, "%s", buff); */
      sprintf(((char*) atributos[i]), "%s", s2);
      break;
    case 'd' : sscanf(s2, "%d", (unsigned int*) Config::atributos[i]); break;
    case 'f' : 
      float aux;
      sscanf(s2, "%f", &aux);
      *((FLT*) Config::atributos[i]) = aux;
      break;
    }
    
  } while (true);
   
  fclose(fp);

  actualizaParametrosInternos();

# undef MAX_LINE_SIZE

  //  printf("...Config::parseaArchivoConf(this = %p)\n", this);
}

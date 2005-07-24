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
#include "gui.h"

// the following tokens will appear in the config file.
char* Config::token[] = {
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
  NULL // NULL terminated array
};


//----------------------------------------------------------------------------

Config::Config() {
  reset(); // set default values.

  // internal parameters associated to each token in the config file.
  void* c_param[] = {
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
  
  memcpy(param, c_param, 14*sizeof(void*));
  format = "sddfdffffddfdd"; // print/scan param formats.
}

//----------------------------------------------------------------------------

// default values
void Config::reset()
{
  sprintf(AUDIO_DEV,"%s","/dev/dsp");

  SAMPLE_RATE   = 8000; // Hz
  OVERSAMPLING  = 5;
  ROOT_FREQUENCY_ERROR = 0;  // Hz
  FFT_SIZE = 256; // samples
  TEMPORAL_WINDOW = 0.1875;  // seconds
  CALCULATION_RATE   = 20; // Hz
  VISUALIZATION_RATE = 25; // Hz
  NOISE_THRESHOLD = 10.0; // dB

  PEAK_NUMBER = 3; // peaks
  PEAK_ORDER = 1;  // samples
  PEAK_REJECTION_RELATION = 20; // dB

  DFT_NUMBER = 2;  // DFTs
  DFT_SIZE   = 15; // samples

  MAX_NR_ITER = 10; // iterations

  //--------------------------------------------------------------------------

  VRP = -0.45; // near to minimum

  updateInternalParameters();
}

//----------------------------------------------------------------------------

void Config::updateInternalParameters()
{
  // derived parameters.
  ROOT_FREQUENCY             = 440.0*pow(2.0, ROOT_FREQUENCY_ERROR/1200.0);
  TEMPORAL_BUFFER_SIZE       = (unsigned int) 
    ceil(TEMPORAL_WINDOW*SAMPLE_RATE/OVERSAMPLING);
  READ_BUFFER_SIZE           = (unsigned int) 
    ceil(SAMPLE_RATE/(CALCULATION_RATE*OVERSAMPLING));
  PEAK_REJECTION_RELATION_UN = pow(10.0, PEAK_REJECTION_RELATION/10.0);
  NOISE_THRESHOLD_UN         = pow(10.0, NOISE_THRESHOLD/10.0);

  if (TEMPORAL_BUFFER_SIZE < FFT_SIZE) {

#   ifdef QUICK_MESSAGES
    char buff[80];
    TEMPORAL_WINDOW = ((double) FFT_SIZE*OVERSAMPLING)/SAMPLE_RATE;
    sprintf(buff, gettext("Temporal buffer is smaller than FFT size.\
 It has been increased to %0.3f seconds\n"), (float)TEMPORAL_WINDOW);
    quick_message(gettext("tuner warning"), buff);
#   endif
    TEMPORAL_BUFFER_SIZE = FFT_SIZE;
  }
  
}

//----------------------------------------------------------------------------

void Config::saveConfigFile(char* file) {

  FILE* fp;

  if ((fp = fopen(file, "w")) == NULL) {
    char buff[100];
    sprintf(buff, "error saving config file %s ", file);
    perror(buff);
    return;
  }

  fprintf(fp, "# Config file automatically created by lingot\n\n");

  for (int i = 0; token[i]; i++) 
    switch (format[i]) {
    case 's': 
      fprintf(fp, "%s = %s\n", token[i], (char*) param[i]); 
      break;
    case 'd': 
      fprintf(fp, "%s = %d\n", token[i], *((unsigned int*) param[i]));
      break;
    case 'f': 
      fprintf(fp, "%s = %0.3f\n", token[i], *((FLT*) param[i])); 
      break;
    }

  fclose(fp);
}

//----------------------------------------------------------------------------

void Config::parseConfigFile( char* file )
{
  FILE* fp;

# define MAX_LINE_SIZE 100
  
  char s1[MAX_LINE_SIZE];
  
  if ((fp = fopen(file, "r")) == NULL) {
    sprintf(s1, 
	    "error opening config file %s, assuming default values ", file);
    perror(s1);
    return;
  }

  int line = 0;

  do {
    
    line++;

    if (!fgets(s1, MAX_LINE_SIZE, fp)) break;;

    //    printf("line %d: %s\n", line, s1);
    
    if (s1[0] == '#') continue;

    //    char* p2 = s1; // puntero sobre la línea leída.
    char* s2;      // contendrá los tokens dentro de la línea.

    // cojo la cadena del atributo.
    /*do {
      s2 = strsep(&p2, " \t=\n");
      } while ((s2) && (strlen(s2) == 0));*/
    s2 = strtok(s1, " \t=\n");

    if (!s2) continue; // blank line.

    int i;
    for (i = 0; Config::token[i]; i++) if (!strcmp(s2, Config::token[i])) {
      break; // found token.
    }

    if (!Config::token[i]) {
      fprintf(stderr, 
	      "warning: parse error at line %i: unknown keyword %s\n", 
	      line, s2);
      continue;
    }

    // cojo el valor del atributo.
    /*do {
      s2 = strsep(&p2, " \t=\n");
      } while ((s2) && (strlen(s2) == 0));*/
    s2 = strtok(NULL, " \t=\n");
    
    if (!s2) {
      fprintf(stderr, "warning: parse error at line %i: value expected\n", 
	      line);
      continue;
    }

    // asign the value to the parameter.
    switch (Config::format[i]) {
    case 's' : 
      /*      char buff[MAX_LINE_SIZE];
	      sscanf(s2, "%s", buff); */
      sprintf(((char*) param[i]), "%s", s2);
      break;
    case 'd' : sscanf(s2, "%d", (unsigned int*) Config::param[i]); break;
    case 'f' : 
      float aux;
      sscanf(s2, "%f", &aux);
      *((FLT*) Config::param[i]) = aux;
      break;
    }
    
  } while (true);
   
  fclose(fp);

  updateInternalParameters();

# undef MAX_LINE_SIZE
}

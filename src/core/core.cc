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

#include <stdio.h>
#include <math.h>
#include <linux/soundcard.h>
#include <string.h>
#include <errno.h>

#ifndef LIB_FFTW
# include "complex.h"
#endif

#include "FFT.h"
#include "core.h"
#include "filter.h"
#include "filters.h"

Analizador::Analizador()
{
  conf.parseConfigFile(CONFIG_FILE);
  iniciar();

  status = true;
  pthread_attr_init(&attrib);
  pthread_create(&thread, &attrib, (void * (*)(void *))ProcessThread, (void *)this); 
}

void Analizador::iniciar()
{
# ifdef LIBSNDOBJ
  A = new SndRTIO(1, SND_INPUT, 512, 512, SHORTSAM_LE, NULL, 
		  conf.READ_BUFFER_SIZE*conf.OVERSAMPLING, conf.SAMPLE_RATE, conf.AUDIO_DEV);
    //    A = new SndRTIO();
# else
  // creamos un manejador de audio.
  A = new audio(1, conf.SAMPLE_RATE, FORMATO_MUESTRA, conf.AUDIO_DEV);
# endif

  // la DEP es simétrica, con almacenar la mitad llega.
  if (conf.FFT_SIZE > 256) {
    dep_fft = new FLT[conf.FFT_SIZE >> 1];
    memset(dep_fft, 0, (conf.FFT_SIZE >> 1)*sizeof(FLT));
  }
  else {
    dep_fft = new FLT[conf.FFT_SIZE];
    memset(dep_fft, 0, conf.FFT_SIZE*sizeof(FLT));
  }

  dep_dft       = new FLT[conf.DFT_SIZE];
  memset(dep_dft, 0, conf.DFT_SIZE*sizeof(FLT));

  diff2_dep_fft = new FLT[conf.FFT_SIZE]; // derivada 2ª de la DEP.
  memset(diff2_dep_fft, 0, conf.FFT_SIZE*sizeof(FLT));

#ifndef LIB_FFTW  
  CreaWN(&conf); // crea los factores de fase de la FFT.
  fft_out    = new CPX[conf.FFT_SIZE]; // señal compleja en dominio frecuencial.
  memset(fft_out, 0, conf.FFT_SIZE*sizeof(CPX));
#else
  fftw_in   = new fftw_complex[conf.FFT_SIZE];
  memset(fftw_in, 0, conf.FFT_SIZE*sizeof(fftw_complex));
  fftw_out  = new fftw_complex[conf.FFT_SIZE];
  memset(fftw_out, 0, conf.FFT_SIZE*sizeof(fftw_complex));
  fftwplan  = fftw_create_plan(conf.FFT_SIZE, FFTW_FORWARD, FFTW_ESTIMATE);
#endif

  // buffer leído de la tarjeta.
  buffer_muestras_leidas = new TIPO_MUESTRA[conf.READ_BUFFER_SIZE*conf.OVERSAMPLING];
  memset(buffer_muestras_leidas, 0, (conf.READ_BUFFER_SIZE*conf.OVERSAMPLING)*sizeof(TIPO_MUESTRA));

  // buffer leído de la tarjeta.
  buffer_leido = new FLT[conf.READ_BUFFER_SIZE*conf.OVERSAMPLING];
  memset(buffer_leido, 0, (conf.READ_BUFFER_SIZE*conf.OVERSAMPLING)*sizeof(FLT));

  // memoria de muestras en tiempo.
  buffer_ventana_temporal = new FLT[conf.TEMPORAL_BUFFER_SIZE]; 
  memset(buffer_ventana_temporal, 0, conf.TEMPORAL_BUFFER_SIZE*sizeof(FLT));

  // filtro Chebyshev de orden 8, wc = pi/sobremuestreo
  filtro_diezmado = new Filtro( 8, 8, filtros[conf.OVERSAMPLING][0], filtros[conf.OVERSAMPLING][1] );

  // ------------------------------------------------------------

  frecuencia = 0.0;
  hay_nueva_conf = false;
}

// -----------------------------------------------------------------------

void Analizador::finalizar()
{
#ifdef LIB_FFTW  
  fftw_destroy_plan(fftwplan);
  delete [] fftw_in;
  delete [] fftw_out;
#else
  DestruyeWN(); // destruye los factores de fase.
  delete [] fft_out;
#endif

  delete    A;

  delete [] dep_fft;
  delete [] dep_dft;  
  delete [] diff2_dep_fft;
  delete [] buffer_muestras_leidas;
  delete [] buffer_leido;
  delete [] buffer_ventana_temporal;

  delete    filtro_diezmado;

  // esperamos por el hilo.
  //  pthread_join(thread, &attrib);
}

// -----------------------------------------------------------------------

Analizador::~Analizador()
{
  finalizar();  
  pthread_attr_destroy(&attrib);
}

// -----------------------------------------------------------------------

void Analizador::cambiaConfig(Config conf)
{
  Analizador::nueva_conf = conf;
  hay_nueva_conf = true;
}

// -----------------------------------------------------------------------

// in y out solapables.
void Analizador::diezmar(FLT* in, FLT* out)
{
  register unsigned int i,j;

  // filtro paso bajo para evitar aliasing.
  filtro_diezmado->filtrarII(conf.READ_BUFFER_SIZE*conf.OVERSAMPLING, in, in);
  
  // compresión.
  for(i = j = 0; i < conf.READ_BUFFER_SIZE; i++, j += conf.OVERSAMPLING)
    out[i] = in[j];
}

// -----------------------------------------------------------------------

// procesa la información leída para calcular la frecuencia fundamental.
void Analizador::procesar()
{
  register unsigned int  i, k;              // para bucles.
  FLT delta_w_FFT = 2.0*M_PI/conf.FFT_SIZE; // resolución en radianes en la FFT.  

# ifdef LIBSNDOBJ
  A->Read();
  for (i = 0; i < conf.OVERSAMPLING*conf.READ_BUFFER_SIZE; i++)
    buffer_leido[i] = A->Output(i);
# else
  if ((A->lee(buffer_muestras_leidas, conf.OVERSAMPLING*conf.READ_BUFFER_SIZE*sizeof(TIPO_MUESTRA))) < 0) {
    //perror("Error al leer muestras");
    return;
  }

  for (i = 0; i < conf.OVERSAMPLING*conf.READ_BUFFER_SIZE; i++)
    buffer_leido[i] = buffer_muestras_leidas[i];
# endif


  /*   dibujos por cortesía de barrio sésamo:

     recién leído:   ----------------------------
                    |bxxxbxxxbxxxbxxxbxxxbxxxbxxx|
		     ----------------------------

		     <----------------------------> READ_BUFFER_SIZE*OVERSAMPLING
  */

  /* desplazo lo que había en el buffer temporal para dejar espacio al
     nuevo trozo de señal leída. El buffer es en realidad una cola. */
  if (conf.TEMPORAL_BUFFER_SIZE > conf.READ_BUFFER_SIZE)
    memcpy(buffer_ventana_temporal, 
	   &buffer_ventana_temporal[conf.READ_BUFFER_SIZE],
	   (conf.TEMPORAL_BUFFER_SIZE - conf.READ_BUFFER_SIZE)*sizeof(FLT));

  /*
    situación anterior del buffer:
     ------------------------------------------
    | xxxxxxxxxxxxxxxxxxxxxx | yyyyy | aaaaaaa |
     ------------------------------------------
                                       <------> READ_BUFFER_SIZE
			      <---------------> FFT_SIZE
     <----------------------------------------> TEMPORAL_BUFFER_SIZE

     nueva situación:

     ------------------------------------------
    | xxxxxxxxxxxxxxxxyyyyaa | aaaaa |         |
     ------------------------------------------   */
    
  /* meto la señal leída de la tarjeta y diezmada al final del buffer. */
  if (conf.OVERSAMPLING > 1)
      diezmar(buffer_leido,
	      &buffer_ventana_temporal[conf.TEMPORAL_BUFFER_SIZE - conf.READ_BUFFER_SIZE]);
  else memcpy(&buffer_ventana_temporal[conf.TEMPORAL_BUFFER_SIZE - conf.READ_BUFFER_SIZE],
	      buffer_leido, conf.READ_BUFFER_SIZE*sizeof(FLT));
  /* ------------------------------------------
    | xxxxxxxxxxxxxxxxyyyyaa | aaaaa | bbbbbbb |
     ------------------------------------------   */

  // ------------------------- PASO A DOMINIO DE FRECUENCIA --------------------------

  FLT _1_N2 = 1.0/(conf.FFT_SIZE*conf.FFT_SIZE); // constante de normalización de la DEP

# ifdef LIB_FFTW
  for (i = 0; i < conf.FFT_SIZE; i++)
    fftw_in[i].re = buffer_ventana_temporal[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE + i];
  
  /* Pasamos a frecuencia. */
  fftw_one(fftwplan, fftw_in, fftw_out);
  
  /* pasamos la transformada (compleja) a su función de módulo al cuadrado normalizada (DEP). */
  for (i = 0; i < ((conf.FFT_SIZE > 256) ? (conf.FFT_SIZE >> 1) : 256); i++)
    dep_fft[i] = (fftw_out[i].re*fftw_out[i].re + fftw_out[i].im*fftw_out[i].im)*_1_N2;
# else
  
  /* Pasamos a frecuencia. */
  FFT(&buffer_ventana_temporal[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE], fft_out, conf.FFT_SIZE);
  
  /* pasamos la transformada (compleja) a su función de módulo al cuadrado normalizada (DEP). */
  for (i = 0; i < ((conf.FFT_SIZE > 256) ? (conf.FFT_SIZE >> 1) : 256); i++)
    dep_fft[i] = (fft_out[i].r*fft_out[i].r + fft_out[i].i*fft_out[i].i)*_1_N2;
# endif

  // trozo representable
  memcpy(X, dep_fft, 256*sizeof(FLT));
      
  // estimación de derivada segunda truncada, para resaltar los picos
  diff2_dep_fft[0] = 0.0;
  for (i = 1; i < conf.FFT_SIZE - 1; i++) {
    diff2_dep_fft[i] = 2.0*dep_fft[i] - dep_fft[i - 1] - dep_fft[i + 1]; // derivada de 2º orden centrada, para evitar retardo de grupo.
    if (diff2_dep_fft[i] < 0.0) diff2_dep_fft[i] = 0.0; // truncamos
  }

  // busco los picos en dicha función.
  int Mi = picoFundamental(diff2_dep_fft, dep_fft, (conf.FFT_SIZE >> 1)); // cojemos el pico fundamental de la señal.

  if (Mi == (signed) (conf.FFT_SIZE >> 1)) {
    frecuencia = 0.0;
    return;
  }
      
  FLT w = (Mi - 1)*delta_w_FFT; // frecuencia de la muestra anterior al pico.

  //  Aproximaremos la frecuencia fundamental haciendo DFT's
  // --------------------------------------------------------
  
  FLT d_w = delta_w_FFT;
  for (k = 0; k < conf.DFT_NUMBER; k++) {
	
    d_w = 2.0*d_w/(conf.DFT_SIZE - 1); // delta de w.
	
    if (k == 0) {
      DEP(&buffer_ventana_temporal[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE],
	  conf.FFT_SIZE, w + d_w, d_w, &dep_dft[1], conf.DFT_SIZE - 2);
      dep_dft[0] = dep_fft[Mi - 1];
      dep_dft[conf.DFT_SIZE - 1] = dep_fft[Mi + 1]; // ya conozco 2 muestras.
    } else
      DEP(&buffer_ventana_temporal[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE],
	  conf.FFT_SIZE, w, d_w, dep_dft, conf.DFT_SIZE);
	
    maximo(dep_dft, conf.DFT_SIZE, Mi); // buscamos el máximo.
	
    w += (Mi - 1)*d_w; // muestra anterior al pico.
  }
      
  w += d_w; // frecuencia aproximada por DFT's.
    
  //  Seguimos aproximando usando el método de Newton-Raphson
  // ---------------------------------------------------------

  FLT wk = -1.0e5; // esto es para que no salte el test de parada al principio.
  FLT wkm1 = w;    // primer iterante.
  FLT d1_DEP, d2_DEP;
    
  for (k = 0; (k < conf.MAX_NR_ITER) && (fabs(wk - wkm1) > 1.0e-8); k++) {
    wk = wkm1;

    // ! ahora uso TODA la ventana temporal para mayor precisión.
    diffs_DEP(buffer_ventana_temporal, conf.TEMPORAL_BUFFER_SIZE, wk, d1_DEP, d2_DEP);
    wkm1 = wk - d1_DEP/d2_DEP;
  }
    
  w = wkm1; // esta es la frecuencia en radianes.
  frecuencia = (w*conf.SAMPLE_RATE)/(2.0*M_PI*conf.OVERSAMPLING); // ésta es la frecuencia analógica encontrada.
}				

void ProcessThread(Analizador* A) {

  while (A->status) {

    // Si se ha requerido una reconfiguración es el momento de atenderla.
    if (A->hay_nueva_conf) {
      A->finalizar();
      A->conf = A->nueva_conf;
      A->iniciar();
    }

    A->procesar(); // proceso los datos leídos. 
  }  

  pthread_exit(NULL);
}

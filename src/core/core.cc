//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004, 2005  Ibán Cereijo Graña, Jairo Chapela Martínez.

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

Core::Core()
{
  conf.parseConfigFile(CONFIG_FILE);
  start();

  status = true;
  pthread_attr_init(&attrib);
  pthread_create(&thread, &attrib, (void * (*)(void *))ProcessThread, (void *)this); 
}

void Core::start()
{
# ifdef LIBSNDOBJ
  A = new SndRTIO(1, SND_INPUT, 512, 512, SHORTSAM_LE, NULL, 
		  conf.READ_BUFFER_SIZE*conf.OVERSAMPLING, conf.SAMPLE_RATE, conf.AUDIO_DEV);
  //    A = new SndRTIO();
# else
  // creates an audio handler.
  A = new audio(1, conf.SAMPLE_RATE, SAMPLE_FORMAT, conf.AUDIO_DEV);
# endif

  // since the SPD is simmetrical, we only store the 1st half.
  if (conf.FFT_SIZE > 256) {
    spd_fft = new FLT[conf.FFT_SIZE >> 1];
    memset(spd_fft, 0, (conf.FFT_SIZE >> 1)*sizeof(FLT));
  }
  else { // if the fft size is 256, we store the whole signal for representation.
    spd_fft = new FLT[conf.FFT_SIZE];
    memset(spd_fft, 0, conf.FFT_SIZE*sizeof(FLT));
  }

  spd_dft       = new FLT[conf.DFT_SIZE];
  memset(spd_dft, 0, conf.DFT_SIZE*sizeof(FLT));

  diff2_spd_fft = new FLT[conf.FFT_SIZE]; // 2nd derivative from SPD.
  memset(diff2_spd_fft, 0, conf.FFT_SIZE*sizeof(FLT));

#ifndef LIB_FFTW  
  createWN(&conf); // creates the phase factors for FFT.
  fft_out    = new CPX[conf.FFT_SIZE]; // complex signal in freq domain.
  memset(fft_out, 0, conf.FFT_SIZE*sizeof(CPX));
#else
  fftw_in   = new fftw_complex[conf.FFT_SIZE];
  memset(fftw_in, 0, conf.FFT_SIZE*sizeof(fftw_complex));
  fftw_out  = new fftw_complex[conf.FFT_SIZE];
  memset(fftw_out, 0, conf.FFT_SIZE*sizeof(fftw_complex));
  fftwplan  = fftw_create_plan(conf.FFT_SIZE, FFTW_FORWARD, FFTW_ESTIMATE);
#endif

  // read buffer from soundcard.
  read_buffer = new SAMPLE_TYPE[conf.READ_BUFFER_SIZE*conf.OVERSAMPLING];
  memset(read_buffer, 0, (conf.READ_BUFFER_SIZE*conf.OVERSAMPLING)*sizeof(SAMPLE_TYPE));

  // read buffer from soundcard in floating point format.
  flt_read_buffer = new FLT[conf.READ_BUFFER_SIZE*conf.OVERSAMPLING];
  memset(flt_read_buffer, 0, (conf.READ_BUFFER_SIZE*conf.OVERSAMPLING)*sizeof(FLT));

  // stored samples.
  temporal_window_buffer = new FLT[conf.TEMPORAL_BUFFER_SIZE]; 
  memset(temporal_window_buffer, 0, conf.TEMPORAL_BUFFER_SIZE*sizeof(FLT));

  // order 8 Chebyshev antialiasing filter, wc = pi/oversampling
  antialiasing_filter = new Filter( 8, 8, filtros[conf.OVERSAMPLING][0], filtros[conf.OVERSAMPLING][1] );

  // ------------------------------------------------------------

  freq = 0.0;
  new_conf_pending = false;
}

// -----------------------------------------------------------------------

void Core::stop()
{
#ifdef LIB_FFTW  
  fftw_destroy_plan(fftwplan);
  delete [] fftw_in;
  delete [] fftw_out;
#else
  destroyWN(); // destroy phase factors.
  delete [] fft_out;
#endif

  delete    A;

  delete [] spd_fft;
  delete [] spd_dft;  
  delete [] diff2_spd_fft;
  delete [] read_buffer;
  delete [] flt_read_buffer;
  delete [] temporal_window_buffer;

  delete    antialiasing_filter;

  // wait for thread.
  //  pthread_join(thread, &attrib);
}

// -----------------------------------------------------------------------

Core::~Core()
{
  stop();  
  pthread_attr_destroy(&attrib);
}

// -----------------------------------------------------------------------

void Core::changeConfig(Config conf)
{
  Core::new_conf = conf;
  new_conf_pending = true;
}

// -----------------------------------------------------------------------

// signal decimation with antialiasing, in & out overlapables.
void Core::decimate(FLT* in, FLT* out)
{
  register unsigned int i,j;

  // low pass filter to avoid aliasing.
  antialiasing_filter->filter(conf.READ_BUFFER_SIZE*conf.OVERSAMPLING, in, in);
  
  // compression.
  for(i = j = 0; i < conf.READ_BUFFER_SIZE; i++, j += conf.OVERSAMPLING)
    out[i] = in[j];
}

// -----------------------------------------------------------------------

void Core::process()
{
  register unsigned int  i, k;              // loop variables.
  FLT delta_w_FFT = 2.0*M_PI/conf.FFT_SIZE; // FFT resolution in rads.  

# ifdef LIBSNDOBJ
  A->Read();
  for (i = 0; i < conf.OVERSAMPLING*conf.READ_BUFFER_SIZE; i++)
    flt_read_buffer[i] = A->Output(i);
# else
  if ((A->read(read_buffer, 
	       conf.OVERSAMPLING*conf.READ_BUFFER_SIZE*sizeof(SAMPLE_TYPE))) 
      < 0) {
    //perror("Error reading samples");
    return;
  }

  for (i = 0; i < conf.OVERSAMPLING*conf.READ_BUFFER_SIZE; i++)
    flt_read_buffer[i] = read_buffer[i];
# endif


  /* just readed:   ----------------------------
                   |bxxxbxxxbxxxbxxxbxxxbxxxbxxx|
		    ----------------------------

		   <----------------------------> READ_BUFFER_SIZE*OVERSAMPLING
  */

  /* we shift the temporal window to leave a hollow where place the new piece
     of data read. The buffer is actually a queue. */
  if (conf.TEMPORAL_BUFFER_SIZE > conf.READ_BUFFER_SIZE)
    memcpy(temporal_window_buffer, 
	   &temporal_window_buffer[conf.READ_BUFFER_SIZE],
	   (conf.TEMPORAL_BUFFER_SIZE - conf.READ_BUFFER_SIZE)*sizeof(FLT));

  /*
    previous buffer situation:
     ------------------------------------------
    | xxxxxxxxxxxxxxxxxxxxxx | yyyyy | aaaaaaa |
     ------------------------------------------
                                       <------> READ_BUFFER_SIZE
			      <---------------> FFT_SIZE
     <----------------------------------------> TEMPORAL_BUFFER_SIZE

     new situation:

     ------------------------------------------
    | xxxxxxxxxxxxxxxxyyyyaa | aaaaa |         |
     ------------------------------------------   */
    
  /* we decimate the read signal and put it at the end of the buffer. */
  if (conf.OVERSAMPLING > 1)
      decimate(flt_read_buffer,
	      &temporal_window_buffer[conf.TEMPORAL_BUFFER_SIZE - conf.READ_BUFFER_SIZE]);
  else memcpy(&temporal_window_buffer[conf.TEMPORAL_BUFFER_SIZE - conf.READ_BUFFER_SIZE],
	      flt_read_buffer, conf.READ_BUFFER_SIZE*sizeof(FLT));
  /* ------------------------------------------
    | xxxxxxxxxxxxxxxxyyyyaa | aaaaa | bbbbbbb |
     ------------------------------------------   */

  // ----------------- TRANSFORMATION TO FREQUENCY DOMAIN ----------------

  FLT _1_N2 = 1.0/(conf.FFT_SIZE*conf.FFT_SIZE); // SPD normalization constant

# ifdef LIB_FFTW
  for (i = 0; i < conf.FFT_SIZE; i++)
    fftw_in[i].re = 
      temporal_window_buffer[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE + i];
  
  // transformation.
  fftw_one(fftwplan, fftw_in, fftw_out);
  
  // esteem of SPD from FFT. (normalized squared module)
  for (i = 0; i < ((conf.FFT_SIZE > 256) ? (conf.FFT_SIZE >> 1) : 256); i++)
    spd_fft[i] = (fftw_out[i].re*fftw_out[i].re +
		  fftw_out[i].im*fftw_out[i].im)*_1_N2;
# else
  
  // transformation.
  FFT(&temporal_window_buffer[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE],
      fft_out, conf.FFT_SIZE);
  
  // esteem of SPD from FFT. (normalized squared module)
  for (i = 0; i < ((conf.FFT_SIZE > 256) ? (conf.FFT_SIZE >> 1) : 256); i++)
    spd_fft[i] = (fft_out[i].r*fft_out[i].r + fft_out[i].i*fft_out[i].i)*_1_N2;
# endif

  // representable piece
  memcpy(X, spd_fft, 256*sizeof(FLT));
      
  // 2nd derivative truncated esteem, to enhance peaks
  diff2_spd_fft[0] = 0.0;
  for (i = 1; i < conf.FFT_SIZE - 1; i++) {
    diff2_spd_fft[i] = 2.0*spd_fft[i] - spd_fft[i - 1] - spd_fft[i + 1]; // centred 2nd order derivative, to avoid group delay.
    if (diff2_spd_fft[i] < 0.0) diff2_spd_fft[i] = 0.0; // truncation
  }

  // peaks search in that signal.
  int Mi = fundamentalPeak(spd_fft, diff2_spd_fft, (conf.FFT_SIZE >> 1)); // take the fundamental peak.

  if (Mi == (signed) (conf.FFT_SIZE >> 1)) {
    freq = 0.0;
    return;
  }
      
  FLT w = (Mi - 1)*delta_w_FFT; // frecuencia de la muestra anterior al pico.

  //  Approximation to fundamental frequency by selective DFTs
  // ---------------------------------------------------------
  
  FLT d_w = delta_w_FFT;
  for (k = 0; k < conf.DFT_NUMBER; k++) {
	
    d_w = 2.0*d_w/(conf.DFT_SIZE - 1); // resolution in rads.
	
    if (k == 0) {
      SPD(&temporal_window_buffer[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE],
	  conf.FFT_SIZE, w + d_w, d_w, &spd_dft[1], conf.DFT_SIZE - 2);
      spd_dft[0] = spd_fft[Mi - 1];
      spd_dft[conf.DFT_SIZE - 1] = spd_fft[Mi + 1]; // 2 samples known.
    } else
      SPD(&temporal_window_buffer[conf.TEMPORAL_BUFFER_SIZE - conf.FFT_SIZE],
	  conf.FFT_SIZE, w, d_w, spd_dft, conf.DFT_SIZE);
	
    max(spd_dft, conf.DFT_SIZE, &Mi); // search the maximum.
	
    w += (Mi - 1)*d_w; // previous sample to the peak.
  }
      
  w += d_w; // approximation by DFTs.
    
  //  Maximum finding by Newton-Raphson
  // -----------------------------------

  FLT wk = -1.0e5;
  FLT wkm1 = w;    // first iterator set to the current approximation.
  FLT d1_SPD, d2_SPD;
    
  for (k = 0; (k < conf.MAX_NR_ITER) && (fabs(wk - wkm1) > 1.0e-8); k++) {
    wk = wkm1;

    // ! we use the WHOLE temporal window for greater precission.
    SPD_diffs(temporal_window_buffer, conf.TEMPORAL_BUFFER_SIZE, wk, &d1_SPD, &d2_SPD);
    wkm1 = wk - d1_SPD/d2_SPD;
  }
    
  w = wkm1; // frequency in rads.
  freq = (w*conf.SAMPLE_RATE)/(2.0*M_PI*conf.OVERSAMPLING); // analog frequency.
}				

void ProcessThread(Core* C) {

  while (C->status) {

    // look for a pending configuration.
    if (C->new_conf_pending) {
      C->stop();
      C->conf = C->new_conf;
      C->start();
    }

    C->process(); // process new data block.
  }  

  pthread_exit(NULL);
}

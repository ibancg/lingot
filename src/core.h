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

#ifndef __CORE_H__
#define __CORE_H__

#include <pthread.h>
#include "defs.h"

#ifdef LIB_FFTW
# include <fftw.h>
#endif

#ifdef LIBSNDOBJ
#  include <SndObj/AudioDefs.h>
#else
#  include "audio.h"
#endif

class CPX;
class IIR;
class Config;

class Core {

public:

	//  -- shared data --
  FLT           freq;   // analog frequency calculated.
  FLT           X[256]; // visual portion of FFT.
	//  -- shared data --

private:  

# ifdef LIBSNDOBJ
  SndRTIO*      A;                       // audio handler.
# else
  audio*        A;                       // audio handler.
# endif

  SAMPLE_TYPE*  read_buffer;
  FLT*          flt_read_buffer;
  FLT*          temporal_window_buffer; // sample memory.

  // spectral power distribution esteem.
  FLT*          spd_fft;
  FLT*          spd_dft;
  FLT*          diff2_spd_fft;

# ifdef LIB_FFTW
	fftw_complex* fftw_in;
	fftw_complex*	fftw_out; // complex signals in time and freq.
	fftw_plan     fftwplan;
# else
  CPX*          fft_out; // complex signal in freq.
# endif

  IIR*          antialiasing_filter; // antialiasing filter for decimation.

  bool          running;

  Config*				conf;       // configuration structure

	// pthread-related  member variables
  pthread_t       thread;
	pthread_attr_t  attr;
	
  //----------------------------------------------------------------

  void	decimate(FLT* in, FLT* out);

  // read and process data to obtain the frequency.
  void	process();
  
  //----------------------------------------------------------------

  // the following methods are implemented in peaks.cc

  /* returns noise threshold at a given frequency w. */
  FLT		noise_threshold(FLT w);
  
  bool	peak(FLT* buffer, int index);
  
  // returns the maximum index.
  void	max(FLT *buffer, int N, int* Mi);
  
  // returns the index of the peak that carries the fundamental freq.
  int		fundamentalPeak(FLT *x, FLT* y, int N);


public:

  Core(Config*);
  ~Core();

	// start process
	void start();
	
	// stop process
	void stop();
	
	// process thread
	void run();
};

#endif //__CORE_H__

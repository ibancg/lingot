//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.
 *
 * This file is part of lingot.
 *
 * lingot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * lingot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINGOT_CORE_H__
#define __LINGOT_CORE_H__

#include <pthread.h>

#include "lingot-defs.h"
#include "lingot-complex.h"
#include "lingot-filter.h"
#include "lingot-config.h"

#ifdef LIB_FFTW
# include <fftw.h>
#endif

#ifdef LIBSNDOBJ
#  include <SndObj/AudioDefs.h>
#else
#  include "lingot-audio.h"
#endif

typedef struct _LingotCore LingotCore;

struct _LingotCore
  {

    //  -- shared data --
    FLT freq; // analog frequency calculated.
    FLT X[256]; // visual portion of FFT.
    //  -- shared data --

# ifdef LIBSNDOBJ
    SndRTIO* audio; // audio handler.
# else
    LingotAudio* audio; // audio handler.
# endif

    SAMPLE_TYPE* read_buffer;
    FLT* flt_read_buffer;
    FLT* temporal_window_buffer; // sample memory.

    // spectral power distribution esteem.
    FLT* spd_fft;
    FLT* spd_dft;
    FLT* diff2_spd_fft;

# ifdef LIB_FFTW
    fftw_complex* fftw_in;
    fftw_complex* fftw_out; // complex signals in time and freq.
    fftw_plan fftwplan;
# else
    LingotComplex* fft_out; // complex signal in freq.
# endif

    LingotFilter* antialiasing_filter; // antialiasing filter for decimation.

    int running;

    LingotConfig* conf; // configuration structure

    // pthread-related  member variables
    pthread_t thread;
    pthread_attr_t attr;
  };

//----------------------------------------------------------------

LingotCore* lingot_core_new(LingotConfig*);
void lingot_core_destroy(LingotCore*);

// start process
void lingot_core_start(LingotCore*);

// stop process
void lingot_core_stop(LingotCore*);

// process thread
void lingot_core_run(LingotCore*);

#endif //__LINGOT_CORE_H__

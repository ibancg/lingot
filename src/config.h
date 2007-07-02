//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.

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

/*
  Configuration class. Determines the tuner's behaviour.
  Some parameters are internal only.
 */
class Config {

private:

  static char* token[];   // config file tokens array.
  void*        param[15]; // parameter pointer array.

  // formats to scan/print tokens from/to the config file.
  static const char* format;

public:
  
  char         AUDIO_DEV[80]; // default "/dev/dsp"
  unsigned int SAMPLE_RATE;   // soundcard sample rate.
  unsigned int OVERSAMPLING;  // oversampling factor.

  // frequency of the root A note (internal parameter).
  FLT          ROOT_FREQUENCY;

  FLT          ROOT_FREQUENCY_ERROR; // deviation of the above root frequency.

  FLT          MIN_FREQUENCY; // minimum valid frequency.

  unsigned int FFT_SIZE; // number of samples of the FFT.

  FLT          CALCULATION_RATE;
  FLT          VISUALIZATION_RATE;

  FLT          TEMPORAL_WINDOW; // duration in seconds of the temporal window.

  // samples stored in the temporal window (internal parameter).
  unsigned int TEMPORAL_BUFFER_SIZE;

  // samples read from soundcard. (internal parameter)
  unsigned int READ_BUFFER_SIZE;

  FLT          NOISE_THRESHOLD;    // dB
  FLT          NOISE_THRESHOLD_UN; // natural units (internal parameter)

  // frequency finding algorithm configuration
  //-------------------------------------------

  unsigned int PEAK_NUMBER; // number of maximum peaks considered.

  // number of adyacent samples needed to consider a peak.
  unsigned int PEAK_ORDER ;

  /* maximum amplitude relation between principal and secondary peaks.
     The max peak doesn't has to be the fundamental frequency carrier if it
     has an amplitude relation with the fundamental considered peak lower than
     this parameter. */
  FLT          PEAK_REJECTION_RELATION;    // dBs
  FLT          PEAK_REJECTION_RELATION_UN; // natural units (internal)

  // DFT approximation
  unsigned int DFT_NUMBER;  // number of DFTs.
  unsigned int DFT_SIZE;    // samples of each DFT.

  // max iterations for Newton-Raphson algorithm.
  unsigned int MAX_NR_ITER;

  //----------------------------------------------------------------------------

  // gauge rest value. (gauge contemplates [-0.5, 0.5])
  FLT          VRP;

  //----------------------------------------------------------------------------

public:
  
  Config();

  // back to default configuration.
  void reset();

  // derivate internal parameters from external ones.
  bool updateInternalParameters();

  void saveConfigFile(char* archivo);
  void parseConfigFile(char* archivo);
};


#endif

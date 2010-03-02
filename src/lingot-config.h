//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2010  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef __LINGOT_CONFIG_H__
#define __LINGOT_CONFIG_H__

#include "lingot-defs.h"

/*
 Configuration class. Determines the tuner's behaviour.
 Some parameters are internal only.
 */

typedef enum audio_system_t {
	AUDIO_SYSTEM_OSS = 0, AUDIO_SYSTEM_ALSA = 1, AUDIO_SYSTEM_JACK = 2
} audio_system_t;

typedef struct _LingotConfig LingotConfig;

struct _LingotConfig {

	audio_system_t audio_system;

	char audio_dev[80]; // default "/dev/dsp"
	char audio_dev_alsa[80]; // default "plughw:0,0"
	unsigned int sample_rate; // soundcard sample rate.
	unsigned int oversampling; // oversampling factor.

	// frequency of the root A note (internal parameter).
	FLT root_frequency;

	FLT root_frequency_error; // deviation of the above root frequency.

	FLT min_frequency; // minimum valid frequency.

	unsigned int fft_size; // number of samples of the FFT.

	FLT calculation_rate;
	FLT visualization_rate;

	FLT temporal_window; // duration in seconds of the temporal window.

	// samples stored in the temporal window (internal parameter).
	unsigned int temporal_buffer_size;

	// samples read from soundcard (internal parameter).
	unsigned int read_buffer_size;

	FLT noise_threshold_db; // dB
	FLT noise_threshold_nu; // natural units (internal parameter)

	// frequency finding algorithm configuration
	//-------------------------------------------

	unsigned int peak_number; // number of maximum peaks considered.

	// number of adjacent samples needed to consider a peak.
	unsigned int peak_half_width;

	/* maximum amplitude relation between principal and secondary peaks.
	 The max peak doesn't has to be the fundamental frequency carrier if it
	 has an amplitude relation with the fundamental considered peak lower than
	 this parameter. */
	FLT peak_rejection_relation_db; // dBs
	FLT peak_rejection_relation_nu; // natural units (internal)

	FLT gain; // dBs
	FLT gain_nu; // natural units (internal)

	// DFT approximation
	unsigned int dft_number; // number of DFTs.
	unsigned int dft_size; // samples of each DFT.

	// max iterations for Newton-Raphson algorithm.
	unsigned int max_nr_iter;

	//----------------------------------------------------------------------------

	// gauge rest value. (gauge contemplates [-0.5, 0.5])
	FLT vr;

	//----------------------------------------------------------------------------
};

// converts an audio_system_t to a string
const char* audio_system_t_to_str(audio_system_t audio_system);
// converts a string to an audio_system_t
audio_system_t str_to_audio_system_t(char* audio_system);

LingotConfig* lingot_config_new();
void lingot_config_destroy(LingotConfig*);

// back to default configuration.
void lingot_config_reset(LingotConfig*);

// derivate internal parameters from external ones.
int lingot_config_update_internal_params(LingotConfig*);

void lingot_config_save(LingotConfig*, char* filename);
void lingot_config_load(LingotConfig*, char* filename);

#endif // __LINGOT_CONFIG_H__

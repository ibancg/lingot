/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include "lingot-config-scale.h"

// configuration parameter identifier
typedef enum LingotConfigParameterId {
	LINGOT_PARAMETER_ID_AUDIO_SYSTEM = 0, //
	LINGOT_PARAMETER_ID_AUDIO_DEV = 1, //
	LINGOT_PARAMETER_ID_AUDIO_DEV_ALSA = 2, //
	LINGOT_PARAMETER_ID_AUDIO_DEV_JACK = 3, //
	LINGOT_PARAMETER_ID_AUDIO_DEV_PULSEAUDIO = 4, //
	LINGOT_PARAMETER_ID_SAMPLE_RATE = 5, //
	LINGOT_PARAMETER_ID_OVERSAMPLING = 6, //
	LINGOT_PARAMETER_ID_ROOT_FREQUENCY_ERROR = 7, //
	LINGOT_PARAMETER_ID_MIN_FREQUENCY = 8, //
	LINGOT_PARAMETER_ID_FFT_SIZE = 9, //
	LINGOT_PARAMETER_ID_TEMPORAL_WINDOW = 10, //
	LINGOT_PARAMETER_ID_NOISE_THRESHOLD = 11, //
	LINGOT_PARAMETER_ID_CALCULATION_RATE = 12, //
	LINGOT_PARAMETER_ID_VISUALIZATION_RATE = 13, //
	LINGOT_PARAMETER_ID_PEAK_NUMBER = 14, //
	LINGOT_PARAMETER_ID_PEAK_HALF_WIDTH = 15, //
	LINGOT_PARAMETER_ID_PEAK_REJECTION_RELATION = 16, //
	LINGOT_PARAMETER_ID_DFT_NUMBER = 17, //
	LINGOT_PARAMETER_ID_DFT_SIZE = 18, //
	LINGOT_PARAMETER_ID_GAIN = 19, //
	LINGOT_PARAMETER_ID_PEAK_ORDER = 20
} LingotConfigParameterId;

// configuration parameter type
typedef enum LingotConfigParameterType {
	LINGOT_PARAMETER_TYPE_STRING,
	LINGOT_PARAMETER_TYPE_INTEGER,
	LINGOT_PARAMETER_TYPE_FLOAT,
	LINGOT_PARAMETER_TYPE_AUDIO_SYSTEM
} LingotConfigParameterType;

typedef struct _LingotConfigParameterSpec LingotConfigParameterSpec;

// configuration parameter specification (id, type, minimum and maximum allowed
// values, ...)
struct _LingotConfigParameterSpec {

	LingotConfigParameterId id;
	LingotConfigParameterType type;
	const char* name;
	const char* units;

	int deprecated;

	int str_max_len;
	int int_min;
	int int_max;
	double float_min;
	double float_max;
};

typedef enum audio_system_t {
	AUDIO_SYSTEM_OSS = 0,
	AUDIO_SYSTEM_ALSA = 1,
	AUDIO_SYSTEM_JACK = 2,
	AUDIO_SYSTEM_PULSEAUDIO = 3
} audio_system_t;

typedef enum window_type_t {
	NONE = 0, //
	HANNING = 1, //
	HAMMING = 2
} window_type_t;

typedef struct _LingotConfig LingotConfig;

// Configuration struct. Determines the tuner behaviour.
// Some parameters are internal only.
struct _LingotConfig {

	audio_system_t audio_system;

	char audio_dev[4][512];
	int sample_rate; // soundcard sample rate.
	unsigned int oversampling; // oversampling factor.

	//	root_frequency_reference_note_t root_frequency_referente_note;
	FLT root_frequency_error; // deviation of the above root frequency.

	FLT min_frequency; // minimum valid frequency.
	FLT max_frequency; // maximum frequency we want to tune.

	unsigned int fft_size; // number of samples of the FFT.

	FLT calculation_rate;
	FLT visualization_rate;

	FLT temporal_window; // duration in seconds of the temporal window.

	// samples stored in the temporal window (internal parameter).
	unsigned int temporal_buffer_size;

	FLT noise_threshold_db; // dB
	FLT noise_threshold_nu; // natural units (internal parameter)

	window_type_t window_type;

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

	// DFT approximation
	unsigned int dft_number; // number of DFTs.
	unsigned int dft_size; // samples of each DFT.

	// max iterations for Newton-Raphson algorithm.
	unsigned int max_nr_iter;

	//----------------------------------------------------------------------------

	// gauge rest value. (gauge contemplates [-0.5, 0.5])
	FLT gauge_rest_value;

	//----------------------------------------------------------------------------

	LingotScale* scale;
};

// converts an audio_system_t to a string
const char* audio_system_t_to_str(audio_system_t audio_system);
// converts a string to an audio_system_t
audio_system_t str_to_audio_system_t(char* audio_system);

void lingot_config_create_parameter_specs();
LingotConfigParameterSpec lingot_config_get_parameter_spec(
		LingotConfigParameterId id);

LingotConfig* lingot_config_new();
void lingot_config_destroy(LingotConfig*);
void lingot_config_copy(LingotConfig* dst, LingotConfig* src);

// back to default configuration.
void lingot_config_restore_default_values(LingotConfig*);

// derivate internal parameters from external ones.
void lingot_config_update_internal_params(LingotConfig*);

void lingot_config_save(LingotConfig*, char* filename);
void lingot_config_load(LingotConfig*, char* filename);

#endif // __LINGOT_CONFIG_H__

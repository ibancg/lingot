/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2018  Iban Cereijo.
 * Copyright (C) 2004-2008  Jairo Chapela.

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

#ifndef __LINGOT_IO_CONFIG_H__
#define __LINGOT_IO_CONFIG_H__

#include "lingot-config.h"

// configuration parameter identifier
typedef enum LingotConfigParameterId {
	LINGOT_PARAMETER_ID_AUDIO_SYSTEM, //
	LINGOT_PARAMETER_ID_AUDIO_DEV, //
	LINGOT_PARAMETER_ID_AUDIO_DEV_ALSA, //
	LINGOT_PARAMETER_ID_AUDIO_DEV_JACK, //
	LINGOT_PARAMETER_ID_AUDIO_DEV_PULSEAUDIO, //
	LINGOT_PARAMETER_ID_ROOT_FREQUENCY_ERROR, //
	LINGOT_PARAMETER_ID_FFT_SIZE, //
	LINGOT_PARAMETER_ID_TEMPORAL_WINDOW, //
	LINGOT_PARAMETER_ID_MIN_SNR, //
	LINGOT_PARAMETER_ID_CALCULATION_RATE, //
	LINGOT_PARAMETER_ID_VISUALIZATION_RATE, //
	LINGOT_PARAMETER_ID_MINIMUM_FREQUENCY, //
	LINGOT_PARAMETER_ID_MAXIMUM_FREQUENCY, //
	// ------- obsolete ---------
	LINGOT_PARAMETER_ID_MIN_FREQUENCY, //
	LINGOT_PARAMETER_ID_GAIN, //
	LINGOT_PARAMETER_ID_NOISE_THRESHOLD, //
	LINGOT_PARAMETER_ID_SAMPLE_RATE, //
	LINGOT_PARAMETER_ID_OVERSAMPLING, //
	LINGOT_PARAMETER_ID_DFT_NUMBER, //
	LINGOT_PARAMETER_ID_DFT_SIZE, //
	LINGOT_PARAMETER_ID_PEAK_ORDER, //
	LINGOT_PARAMETER_ID_PEAK_NUMBER, //
	LINGOT_PARAMETER_ID_PEAK_HALF_WIDTH, //
	LINGOT_PARAMETER_ID_PEAK_REJECTION_RELATION, //
} LingotConfigParameterId;

// configuration parameter type
typedef enum LingotConfigParameterType {
	LINGOT_PARAMETER_TYPE_STRING,
	LINGOT_PARAMETER_TYPE_INTEGER,
	LINGOT_PARAMETER_TYPE_FLOAT,
	LINGOT_PARAMETER_TYPE_AUDIO_SYSTEM
} LingotConfigParameterType;

typedef struct _LingotConfigParameterSpec LingotConfigParameterSpec;

// configuration parameter specification (id, type, minimum and maximum allowed values, ...)
struct _LingotConfigParameterSpec {

	LingotConfigParameterId id;
	LingotConfigParameterType type;
	const char* name;
	const char* units;

	int deprecated;

	unsigned int str_max_len;
	int int_min;
	int int_max;
	double float_min;
	double float_max;
};


// converts an audio_system_t to a string
const char* audio_system_t_to_str(audio_system_t audio_system);
// converts a string to an audio_system_t
audio_system_t str_to_audio_system_t(char* audio_system);

void lingot_io_config_create_parameter_specs();
LingotConfigParameterSpec lingot_io_config_get_parameter_spec(LingotConfigParameterId id);

void lingot_io_config_save(LingotConfig*, const char* filename);
void lingot_io_config_load(LingotConfig*, const char* filename);

#endif // __LINGOT_IO_CONFIG_H__

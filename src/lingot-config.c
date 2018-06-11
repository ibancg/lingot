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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <locale.h>

#include "lingot-defs.h"
#include "lingot-config.h"
#include "lingot-config-scale.h"
#include "lingot-msg.h"
#include "lingot-i18n.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))


void lingot_config_new(LingotConfig* config) {

	config->max_nr_iter = 10; // iterations
	config->window_type = HAMMING;
	config->optimize_internal_parameters = 0;
	lingot_config_scale_new(&config->scale);
}

void lingot_config_destroy(LingotConfig* config) {
	lingot_config_scale_destroy(&config->scale);
}

void lingot_config_copy(LingotConfig* dst, const LingotConfig* src) {
	*dst = *src;
	lingot_config_scale_new(&dst->scale); // null scale that will be destroyed in the copy below
	lingot_config_scale_copy(&dst->scale, &src->scale);
}

//----------------------------------------------------------------------------

void lingot_config_restore_default_values(LingotConfig* config) {

#if defined(DEFAULT_AUDIO_SYSTEM_OSS)
	config->audio_system = AUDIO_SYSTEM_OSS;
#elif defined(DEFAULT_AUDIO_SYSTEM_JACK)
	config->audio_system = AUDIO_SYSTEM_JACK;
#elif defined(DEFAULT_AUDIO_SYSTEM_PULSEAUDIO)
	config->audio_system = AUDIO_SYSTEM_PULSEAUDIO;
#else
	config->audio_system = AUDIO_SYSTEM_ALSA;
#endif
	sprintf(config->audio_dev[AUDIO_SYSTEM_OSS], "%s", "/dev/dsp");
	sprintf(config->audio_dev[AUDIO_SYSTEM_ALSA], "%s", "default");
	sprintf(config->audio_dev[AUDIO_SYSTEM_JACK], "%s", "default");
	sprintf(config->audio_dev[AUDIO_SYSTEM_PULSEAUDIO], "%s", "default");

	config->sample_rate = 44100; // Hz
	config->oversampling = 21;
	config->root_frequency_error = 0.0; // Hz
	config->min_frequency = 82.407; // Hz (E2)
	config->max_frequency = 329.6276; // Hz (E4)
	config->optimize_internal_parameters = 0;

	config->fft_size = 512; // samples
	config->temporal_window = 0.3; // seconds
	config->calculation_rate = 15.0; // Hz
	config->visualization_rate = 24.0; // Hz
	config->min_overall_SNR = 20.0; // dB

	config->peak_number = 8; // peaks
	config->peak_half_width = 1; // samples

	//--------------------------------------------------------------------------

	lingot_config_scale_restore_default_values(&config->scale);
	lingot_config_update_internal_params(config);
}

//----------------------------------------------------------------------------

void lingot_config_update_internal_params(LingotConfig* config) {

	// derived parameters.

	config->internal_min_frequency = 0.8 * config->min_frequency;
	config->internal_max_frequency = 3.1 * config->max_frequency;

	if (config->internal_min_frequency < 0) {
		config->internal_min_frequency = 0;
	}
	if (config->internal_max_frequency < 500) {
		config->internal_max_frequency = 500;
	}
	if (config->internal_max_frequency > 20000) {
		config->internal_max_frequency = 20000;
	}

	config->oversampling = floor(
			0.5 * config->sample_rate / config->internal_max_frequency);
	if (config->oversampling < 1) {
		config->oversampling = 1;
	}

	// TODO: tune this parameters
	unsigned int fft_size = 512;
	if (config->internal_max_frequency > 5000) {
		fft_size = 1024;
	}
	FLT temporal_window = 1.0 * config->fft_size * config->oversampling
			/ config->sample_rate;
	if (temporal_window < 0.3) {
		temporal_window = 0.3;
	}
	if (config->optimize_internal_parameters) {
		config->fft_size = fft_size;
		config->temporal_window = temporal_window;
	} else {
		// if the governed parameters happen to be the same as the one we would
		// suggest, then we will indeed suggest them.
		config->optimize_internal_parameters = (config->fft_size == fft_size) &&
				(config->temporal_window == temporal_window);
	}

	config->temporal_buffer_size = (unsigned int) ceil(
			config->temporal_window * config->sample_rate
			/ config->oversampling);

	config->min_SNR = 0.5 * config->min_overall_SNR;
	config->peak_half_width = (config->fft_size > 256) ? 2 : 1;

	if (config->scale.notes == 1) {
		config->scale.max_offset_rounded = 1200.0;
	} else {
		int i;
		FLT max_offset = 0.0;
		for (i = 1; i < config->scale.notes; i++) {
			max_offset = MAX(max_offset, config->scale.offset_cents[i]
					- config->scale.offset_cents[i - 1]);
		}
		config->scale.max_offset_rounded = max_offset;
	}

	config->gauge_rest_value = -0.45 * config->scale.max_offset_rounded;
}


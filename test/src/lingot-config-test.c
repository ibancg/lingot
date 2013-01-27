/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2013  Ibán Cereijo Graña
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
#include <math.h>

#include "lingot-msg.h"
#include "lingot-msg.c"
#include "lingot-config-scale.h"
#include "lingot-config-scale.c"
#include "lingot-config.h"
#include "lingot-config.c"

int lingot_config_test() {

	lingot_config_create_parameter_specs();
	LingotConfig* config = lingot_config_new();

	assert((config != NULL));

	lingot_config_load(config, "resources/lingot-001.conf");

	assert((config->audio_system == AUDIO_SYSTEM_PULSEAUDIO));
	assert(
			!strcmp(config->audio_dev[config->audio_system], "alsa_input.pci-0000_00_1b.0.analog-stereo"));
	assert((config->sample_rate == 44100));
	assert((config->oversampling == 25));
	assert((config->root_frequency_error == 0.0));
	assert((config->min_frequency == 15.0));
	assert((config->sample_rate == 44100));
	assert((config->fft_size == 512));
	assert((config->temporal_window == ((FLT) 0.32)));
	assert((config->noise_threshold_db == 20.0));
	assert((config->calculation_rate == 20.0));
	assert((config->visualization_rate == 30.0));
	assert((config->peak_number == 3));
	assert((config->peak_half_width == 1));
	assert((config->peak_rejection_relation_db == 20.0));
	assert((config->dft_number == 2));
	assert((config->dft_size == 15));
	assert(config->gain == -21.9);

	lingot_config_destroy(config);

	puts("done.");
	return 0;
}

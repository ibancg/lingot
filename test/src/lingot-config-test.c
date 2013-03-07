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

#include "lingot-test.h"

#include "lingot-config-scale.h"
#include "lingot-config.h"

void lingot_config_test() {

	lingot_config_create_parameter_specs();
	LingotConfig* config = lingot_config_new();

	CU_ASSERT_PTR_NOT_NULL_FATAL(config);

	lingot_config_load(config, "resources/lingot-001.conf");

	CU_ASSERT_EQUAL(config->audio_system, AUDIO_SYSTEM_PULSEAUDIO);
	CU_ASSERT(
			!strcmp(config->audio_dev[config->audio_system], "alsa_input.pci-0000_00_1b.0.analog-stereo"));
	CU_ASSERT_EQUAL(config->sample_rate, 44100);
	CU_ASSERT_EQUAL(config->oversampling, 25);
	CU_ASSERT_EQUAL(config->root_frequency_error, 0.0);
	CU_ASSERT_EQUAL(config->min_frequency, 15.0);
	CU_ASSERT_EQUAL(config->sample_rate, 44100);
	CU_ASSERT_EQUAL(config->fft_size, 512);
	CU_ASSERT_EQUAL(config->temporal_window, ((FLT) 0.32));
	CU_ASSERT_EQUAL(config->min_overall_SNR, 20.0);
	CU_ASSERT_EQUAL(config->calculation_rate, 20.0);
	CU_ASSERT_EQUAL(config->visualization_rate, 30.0);
	CU_ASSERT_EQUAL(config->peak_number, 3);
	CU_ASSERT_EQUAL(config->peak_half_width, 1);

	lingot_config_destroy(config);
}

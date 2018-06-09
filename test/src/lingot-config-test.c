/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2013  Iban Cereijo
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

	// old file with obsolete options
	// ------------------------------

	lingot_config_load(config, "resources/lingot-001.conf");

	CU_ASSERT_EQUAL(config->audio_system, AUDIO_SYSTEM_PULSEAUDIO);
	CU_ASSERT(
			!strcmp(config->audio_dev[config->audio_system],
					"alsa_input.pci-0000_00_1b.0.analog-stereo"));
	CU_ASSERT_EQUAL(config->sample_rate, 44100);
	CU_ASSERT_EQUAL(config->root_frequency_error, 0.0);
	CU_ASSERT_EQUAL(config->sample_rate, 44100);
	CU_ASSERT_EQUAL(config->fft_size, 512);
	CU_ASSERT_EQUAL(config->min_overall_SNR, 20.0);
	CU_ASSERT_EQUAL(config->calculation_rate, 20.0);
	CU_ASSERT_EQUAL(config->visualization_rate, 30.0);

	// recent file
	// -----------

	lingot_config_load(config, "resources/lingot-0_9_2b8.conf");

	CU_ASSERT_EQUAL(config->audio_system, AUDIO_SYSTEM_PULSEAUDIO);
	CU_ASSERT(!strcmp(config->audio_dev[config->audio_system], "default"));
	CU_ASSERT_EQUAL(config->sample_rate, 44100);
	CU_ASSERT_EQUAL(config->root_frequency_error, 0.0);
	CU_ASSERT_EQUAL(config->fft_size, 512);
	CU_ASSERT_EQUAL(config->sample_rate, 44100);
	CU_ASSERT_EQUAL(config->temporal_window, 0.3);
	CU_ASSERT_EQUAL(config->min_overall_SNR, 20.0);
	CU_ASSERT_EQUAL(config->calculation_rate, 15.0);
	CU_ASSERT_EQUAL(config->visualization_rate, 24.0);
	CU_ASSERT_EQUAL(config->min_frequency, 82.41);
	CU_ASSERT_EQUAL(config->max_frequency, 329.63);

	CU_ASSERT(!strcmp(config->scale->name, "Default equal-tempered scale"));
	CU_ASSERT_EQUAL(config->scale->notes, 12);
	CU_ASSERT_EQUAL(config->scale->base_frequency, 261.625565);
	CU_ASSERT(!strcmp(config->scale->note_name[1], "C#"));
	CU_ASSERT(!strcmp(config->scale->note_name[11], "B"));

	CU_ASSERT_EQUAL(config->scale->offset_ratios[0][0], 1);
	CU_ASSERT_EQUAL(config->scale->offset_ratios[1][0], 1); // defined as ratio
	CU_ASSERT_EQUAL(config->scale->offset_cents[0], 0.0);

	CU_ASSERT_EQUAL(config->scale->offset_ratios[0][1], -1);
	CU_ASSERT_EQUAL(config->scale->offset_ratios[1][1], -1); // not defined as ratio
	CU_ASSERT_EQUAL(config->scale->offset_cents[1], 100.0);  // defined as shift in cents

	CU_ASSERT_EQUAL(config->scale->offset_ratios[0][11], -1);
	CU_ASSERT_EQUAL(config->scale->offset_ratios[1][11], -1); // not defined as ratio
	CU_ASSERT_EQUAL(config->scale->offset_cents[11], 1100.0); // defined as shift in cents

	lingot_config_destroy(config);
}

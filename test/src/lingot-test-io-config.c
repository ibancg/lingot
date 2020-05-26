/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2013-2020  Iban Cereijo
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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "lingot-test.h"

#include "lingot-audio.h"
#include "lingot-config-scale.h"
#include "lingot-io-config.h"

#include <memory.h>
#include <locale.h>

#ifndef LINGOT_TEST_RESOURCES_PATH
#define LINGOT_TEST_RESOURCES_PATH "test/"
#endif

static void check_last_configs(const lingot_config_t* config) {
#ifdef PULSEAUDIO
    CU_ASSERT_EQUAL(config->audio_system_index, lingot_audio_system_find_by_name("PulseAudio"));
    CU_ASSERT(!strcmp(config->audio_dev[config->audio_system_index], "alsa_input.pci-0000_00_1b.0.analog-stereo"));
#endif
    CU_ASSERT_EQUAL(config->sample_rate, 44100);
    CU_ASSERT_EQUAL(config->root_frequency_error, 0.0);
    CU_ASSERT_EQUAL(config->fft_size, 512);
    CU_ASSERT_EQUAL(config->sample_rate, 44100);
    CU_ASSERT_EQUAL(config->temporal_window, 0.3);
    CU_ASSERT_EQUAL(config->min_overall_SNR, 20.0);
    CU_ASSERT_EQUAL(config->calculation_rate, 15.0);
    CU_ASSERT_EQUAL(config->min_frequency, 82.41);
    CU_ASSERT_EQUAL(config->max_frequency, 329.63);

    CU_ASSERT(!strcmp(config->scale.name, "Default equal-tempered scale"));
    CU_ASSERT_EQUAL(config->scale.notes, 12);
    CU_ASSERT_EQUAL(config->scale.base_frequency, 261.625565);
    CU_ASSERT(!strcmp(config->scale.note_name[1], "C#"));
    CU_ASSERT(!strcmp(config->scale.note_name[11], "B"));

    CU_ASSERT_EQUAL(config->scale.offset_ratios[0][0], 1);
    CU_ASSERT_EQUAL(config->scale.offset_ratios[1][0], 1); // defined as ratio
    CU_ASSERT_EQUAL(config->scale.offset_cents[0], 0.0);

    CU_ASSERT_EQUAL(config->scale.offset_ratios[0][1], -1);
    CU_ASSERT_EQUAL(config->scale.offset_ratios[1][1], -1); // not defined as ratio
    CU_ASSERT_EQUAL(config->scale.offset_cents[1], 100.5);  // defined as shift in cents

    CU_ASSERT_EQUAL(config->scale.offset_ratios[0][11], -1);
    CU_ASSERT_EQUAL(config->scale.offset_ratios[1][11], -1); // not defined as ratio
    CU_ASSERT_EQUAL(config->scale.offset_cents[11], 1100.1); // defined as shift in cents
}

void lingot_test_io_config(void) {

    // check if the files are there
    const char* filename = LINGOT_TEST_RESOURCES_PATH "resources/lingot-001.conf";

    FILE *file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
    } else {
        fprintf(stderr, "%s\n", "warning: test resource files not available");
        return; // we allow the test resources to be not present for the distcheck.
    }

    lingot_io_config_create_parameter_specs();
    lingot_config_t _config;
    lingot_config_t* config = &_config;
    lingot_config_new(config);

    CU_ASSERT_PTR_NOT_NULL_FATAL(config);

    // old file with obsolete options
    // ------------------------------

    int ok;
    ok = lingot_io_config_load(config, LINGOT_TEST_RESOURCES_PATH "resources/lingot-001.conf");
    CU_ASSERT(ok);

    CU_ASSERT_EQUAL(config->audio_system_index, lingot_audio_system_find_by_name("PulseAudio"));
    CU_ASSERT(!strcmp(config->audio_dev[config->audio_system_index],
              "alsa_input.pci-0000_00_1b.0.analog-stereo"));
    CU_ASSERT_EQUAL(config->sample_rate, 44100);
    CU_ASSERT_EQUAL(config->root_frequency_error, 0.0);
    CU_ASSERT_EQUAL(config->sample_rate, 44100);
    CU_ASSERT_EQUAL(config->fft_size, 512);
    CU_ASSERT_EQUAL(config->min_overall_SNR, 20.0);
    CU_ASSERT_EQUAL(config->calculation_rate, 20.0);

    // recent file
    // -----------

    ok = lingot_io_config_load(config, LINGOT_TEST_RESOURCES_PATH "resources/lingot-0_9_2b8.conf");
    CU_ASSERT(ok);

#ifdef PULSEAUDIO
    CU_ASSERT_EQUAL(config->audio_system_index, lingot_audio_system_find_by_name("PulseAudio"));
    CU_ASSERT(!strcmp(config->audio_dev[config->audio_system_index], "default"));
#endif
    CU_ASSERT_EQUAL(config->sample_rate, 44100);
    CU_ASSERT_EQUAL(config->root_frequency_error, 0.0);
    CU_ASSERT_EQUAL(config->fft_size, 512);
    CU_ASSERT_EQUAL(config->sample_rate, 44100);
    CU_ASSERT_EQUAL(config->temporal_window, 0.3);
    CU_ASSERT_EQUAL(config->min_overall_SNR, 20.0);
    CU_ASSERT_EQUAL(config->calculation_rate, 15.0);
    CU_ASSERT_EQUAL(config->min_frequency, 82.41);
    CU_ASSERT_EQUAL(config->max_frequency, 329.63);

    CU_ASSERT(!strcmp(config->scale.name, "Default equal-tempered scale"));
    CU_ASSERT_EQUAL(config->scale.notes, 12);
    CU_ASSERT_EQUAL(config->scale.base_frequency, 261.625565);
    CU_ASSERT(!strcmp(config->scale.note_name[1], "C#"));
    CU_ASSERT(!strcmp(config->scale.note_name[11], "B"));

    CU_ASSERT_EQUAL(config->scale.offset_ratios[0][0], 1);
    CU_ASSERT_EQUAL(config->scale.offset_ratios[1][0], 1); // defined as ratio
    CU_ASSERT_EQUAL(config->scale.offset_cents[0], 0.0);

    CU_ASSERT_EQUAL(config->scale.offset_ratios[0][1], -1);
    CU_ASSERT_EQUAL(config->scale.offset_ratios[1][1], -1); // not defined as ratio
    CU_ASSERT_EQUAL(config->scale.offset_cents[1], 100.0);  // defined as shift in cents

    CU_ASSERT_EQUAL(config->scale.offset_ratios[0][11], -1);
    CU_ASSERT_EQUAL(config->scale.offset_ratios[1][11], -1); // not defined as ratio
    CU_ASSERT_EQUAL(config->scale.offset_cents[11], 1100.0); // defined as shift in cents

    // -----------

    ok = lingot_io_config_load(config, LINGOT_TEST_RESOURCES_PATH "resources/lingot-1_0_2b.conf");
    CU_ASSERT(ok);

    check_last_configs(config);

    // we check that we can save and load it again

    // although the use of tmpnam() is flagged as 'dangerous', it is the only
    // standard way to get the temp file name with full path, and it is used
    // only during the tests anyway.
    char temp_filename_buffer[L_tmpnam];
    char* temp_filename;
    temp_filename = tmpnam(temp_filename_buffer);
    if (!temp_filename) {
        fprintf(stderr, "%s\n", "warning: temporary file cannot be created");
        return; // we allow to continue with the distcheck.
    }

    lingot_io_config_save(config, temp_filename);
    ok = lingot_io_config_load(config, temp_filename);
    CU_ASSERT(ok);
    check_last_configs(config);

    ok = !remove(temp_filename);
    CU_ASSERT(ok);

    // -----------

    ok = lingot_io_config_load(config, LINGOT_TEST_RESOURCES_PATH "resources/lingot-1_1_0.conf");

    CU_ASSERT(ok);

    check_last_configs(config);

    // we check that we can save and load it again

    temp_filename = tmpnam(temp_filename_buffer);
    if (!temp_filename) {
        fprintf(stderr, "%s\n", "warning: temporary file cannot be created");
        return; // we allow to continue with the distcheck.
    }
    lingot_io_config_save(config, temp_filename);
    ok = lingot_io_config_load(config, temp_filename);
    CU_ASSERT(ok);
    check_last_configs(config);

    ok = !remove(temp_filename);
    CU_ASSERT(ok);

    // -----------

    lingot_config_destroy(config);
}

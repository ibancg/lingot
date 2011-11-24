/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2011  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#define N_OPTIONS 22

// the following tokens will appear in the config file. The options after | are deprecated options.
const char* options[] = { "AUDIO_SYSTEM", "AUDIO_DEV", "AUDIO_DEV_ALSA",
		"AUDIO_DEV_JACK", "AUDIO_DEV_PULSEAUDIO", "SAMPLE_RATE", "OVERSAMPLING",
		"ROOT_FREQUENCY_ERROR", "MIN_FREQUENCY", "FFT_SIZE", "TEMPORAL_WINDOW",
		"NOISE_THRESHOLD", "CALCULATION_RATE", "VISUALIZATION_RATE",
		"PEAK_NUMBER", "PEAK_HALF_WIDTH", "PEAK_REJECTION_RELATION",
		"DFT_NUMBER", "DFT_SIZE", "GAIN", "|", "PEAK_ORDER", NULL // NULL terminated array
		};

// units, in order to have a more human-readable config file
const char* units[] = { NULL, NULL, NULL, NULL, NULL, "Hz", NULL, "cents", "Hz",
		"samples", "seconds", "dB", "Hz", "Hz", "peaks", "samples", "dB",
		"DFTs", "samples", NULL, NULL, NULL };

// print/scan param formats.
const char* option_formats = "mssssddffdffffddfddf|d";

const char* audio_systems[] = { "OSS", "ALSA", "JACK", "PulseAudio", NULL };

// converts an audio_system_t to a string
const char* audio_system_t_to_str(audio_system_t audio_system) {
	return audio_systems[audio_system];
}

// converts a string to an audio_system_t
audio_system_t str_to_audio_system_t(char* audio_system) {
	audio_system_t result = -1;
	int i;
	for (i = 0; audio_systems[i] != NULL; i++) {
		if (!strcmp(audio_system, audio_systems[i])) {
			result = i;
			break;
		}
	}
	return result;
}

//----------------------------------------------------------------------------

LingotConfig* lingot_config_new() {

	LingotConfig* config = malloc(sizeof(LingotConfig));

	config->max_nr_iter = 10; // iterations
	config->window_type = HAMMING;
	config->scale = lingot_config_scale_new();
	return config;
}

void lingot_config_destroy(LingotConfig* config) {
	lingot_config_scale_destroy(config->scale);
	free(config->scale);
	free(config);
}

void lingot_config_copy(LingotConfig* dst, LingotConfig* src) {
	LingotScale* dst_scale = dst->scale;
	*dst = *src;
	dst->scale = dst_scale;
	lingot_config_scale_copy(dst->scale, src->scale);
}

//----------------------------------------------------------------------------

void lingot_config_restore_default_values(LingotConfig* config) {

	config->audio_system = AUDIO_SYSTEM_ALSA;
	sprintf(config->audio_dev[AUDIO_SYSTEM_OSS], "%s", "/dev/dsp");
	sprintf(config->audio_dev[AUDIO_SYSTEM_ALSA], "%s", "default");
	sprintf(config->audio_dev[AUDIO_SYSTEM_JACK], "%s", "default");
	sprintf(config->audio_dev[AUDIO_SYSTEM_PULSEAUDIO], "%s", "default");

	config->sample_rate = 44100; // Hz
	config->oversampling = 25;
	config->root_frequency_error = 0; // Hz
	config->min_frequency = 15; // Hz
	config->fft_size = 512; // samples
	config->temporal_window = 0.32; // seconds
	config->calculation_rate = 20; // Hz
	config->visualization_rate = 30; // Hz
	config->noise_threshold_db = 20.0; // dB
	config->gain = 0;

	config->peak_number = 3; // peaks
	config->peak_half_width = 1; // samples
	config->peak_rejection_relation_db = 20; // dB

	config->dft_number = 2; // DFTs
	config->dft_size = 15; // samples

	//--------------------------------------------------------------------------

	lingot_config_scale_restore_default_values(config->scale);
	lingot_config_update_internal_params(config);
}

//----------------------------------------------------------------------------

void lingot_config_update_internal_params(LingotConfig* config) {

	// derived parameters.
	config->temporal_buffer_size = (unsigned int) ceil(
			config->temporal_window * config->sample_rate
					/ config->oversampling);
	config->peak_rejection_relation_nu = pow(10.0,
			config->peak_rejection_relation_db / 10.0);
	config->noise_threshold_nu = pow(10.0, config->noise_threshold_db / 10.0);
	config->gain_nu = pow(10.0, config->gain / 20.0);

	LingotScale* scale = config->scale;
	if (scale->notes == 1) {
		scale->max_offset_rounded = 1200.0;
	} else {
		int i;
		FLT max_offset = 0.0;
		for (i = 1; i < scale->notes; i++) {
			max_offset = MAX(max_offset, scale->offset_cents[i]
					- scale->offset_cents[i - 1]);
		}
		scale->max_offset_rounded = max_offset;
	}

	config->gauge_rest_value = -0.45 * scale->max_offset_rounded;
	sprintf(config->audio_dev[AUDIO_SYSTEM_JACK], "%s", "default");
}

//----------------------------------------------------------------------------

// internal parameters mapped to each token in the config file.
static void lingot_config_map_parameters(LingotConfig* config, void* params[]) {
	void* c_params[] = { &config->audio_system,
			&config->audio_dev[AUDIO_SYSTEM_OSS],
			&config->audio_dev[AUDIO_SYSTEM_ALSA],
			&config->audio_dev[AUDIO_SYSTEM_JACK],
			&config->audio_dev[AUDIO_SYSTEM_PULSEAUDIO], &config->sample_rate,
			&config->oversampling, &config->root_frequency_error,
			&config->min_frequency, &config->fft_size, &config->temporal_window,
			&config->noise_threshold_db, &config->calculation_rate,
			&config->visualization_rate, &config->peak_number,
			&config->peak_half_width, &config->peak_rejection_relation_db,
			&config->dft_number, &config->dft_size, &config->gain, NULL,
			&config->peak_half_width };

	memcpy(params, c_params, N_OPTIONS * sizeof(void*));
}

void lingot_config_save(LingotConfig* config, char* filename) {
	unsigned int i;
	FILE* fp;
	char* lc_all;
	void* params[N_OPTIONS]; // parameter pointer array.
	void* param = NULL;
	const char* option = NULL;
	char buff[80];

	lingot_config_map_parameters(config, params);

	lc_all = setlocale(LC_ALL, NULL);
	// duplicate the string, as the next call to setlocale will destroy it
	if (lc_all)
		lc_all = strdup(lc_all);
	setlocale(LC_ALL, "C");

	if ((fp = fopen(filename, "w")) == NULL) {
		char buff[100];
		sprintf(buff, "error saving config file %s ", filename);
		perror(buff);
		return;
	}

	fprintf(fp, "# Config file automatically created by lingot %s\n\n",
			VERSION);

	for (i = 0; strcmp(options[i], "|"); i++) {

		option = options[i];
		param = params[i];

		switch (option_formats[i]) {
		case 's':
			fprintf(fp, "%s = %s", option, (char*) param);
			break;
		case 'd':
			fprintf(fp, "%s = %d", option, *((unsigned int*) param));
			break;
		case 'f':
			fprintf(fp, "%s = %0.3f", option, *((FLT*) param));
			break;
		case 'm':
			if (!strcmp("AUDIO_SYSTEM", option)) {
				fprintf(fp, "%s = %s", option,
						audio_system_t_to_str(*((audio_system_t*) param)));
			}
			break;
		}

		if (units[i] != NULL) {
			fprintf(fp, " # %s", units[i]);
		}

		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");
	fprintf(fp, "SCALE = {\n");
	fprintf(fp, "NAME = %s\n", config->scale->name);
	fprintf(fp, "BASE_FREQUENCY = %f\n", config->scale->base_frequency);
	fprintf(fp, "NOTE_COUNT = %d\n", config->scale->notes);
	fprintf(fp, "NOTES = {\n");

	for (i = 0; i < config->scale->notes; i++) {
		lingot_config_scale_format_shift(buff, config->scale->offset_cents[i],
				config->scale->offset_ratios[0][i],
				config->scale->offset_ratios[1][i]);
		fprintf(fp, "%s\t%s\n", config->scale->note_name[i], buff);
	}

	fprintf(fp, "}\n"), fprintf(fp, "}\n"),

	fclose(fp);

	if (lc_all) {
		setlocale(LC_ALL, lc_all);
		free(lc_all);
	}
}

//----------------------------------------------------------------------------

void lingot_config_load(LingotConfig* config, char* filename) {
	FILE* fp;
	float aux;
	int line;
	int option_index;
	int deprecated_option = 0;
	char* char_buffer_pointer;
	const static char* delim = " \t=\n";
	const static char* delim2 = " \t\n";
	void* params[N_OPTIONS]; // parameter pointer array.
	void* param = NULL;
	const char* option = NULL;
	int reading_scale = 0;
	char* nl;
	int parse_errors = 0;
	int command_count = 0;

	// restore default values for non specified parameters
	lingot_config_restore_default_values(config);

	lingot_config_map_parameters(config, params);

	static const unsigned int max_line_size = 512;
	char char_buffer[max_line_size];

	if ((fp = fopen(filename, "r")) == NULL) {
		sprintf(char_buffer,
				"error opening config file %s, assuming default values ",
				filename);
		perror(char_buffer);
		return;
	}

	line = 0;

	for (;;) {

		line++;

		if (!fgets(char_buffer, max_line_size, fp))
			break;;

		if (char_buffer[0] == '#')
			continue;

		// tokens into the line.
		char_buffer_pointer = strtok(char_buffer, delim);

		if (!char_buffer_pointer)
			continue; // blank line.

		if (!strcmp(char_buffer_pointer, "SCALE")) {
			reading_scale = 1;
			config->scale = lingot_config_scale_new();
			command_count++;
			continue;
		}

		if (reading_scale) {

			if (!strcmp(char_buffer_pointer, "NAME")) {
				char_buffer_pointer += 4;
				while (1) {
					nl = strchr(delim, *char_buffer_pointer);
					if (!nl)
						break;
					char_buffer_pointer++;
				}
				nl = strrchr(char_buffer_pointer, '\r');
				if (nl)
					*nl = '\0';
				nl = strrchr(char_buffer_pointer, '\n');
				if (nl)
					*nl = '\0';
				config->scale->name = strdup(char_buffer_pointer);
				continue;
			}
			if (!strcmp(char_buffer_pointer, "BASE_FREQUENCY")) {
				char_buffer_pointer = strtok(NULL, delim);
				sscanf(char_buffer_pointer, "%lg",
						&config->scale->base_frequency);
				continue;
			}
			if (!strcmp(char_buffer_pointer, "NOTE_COUNT")) {
				char_buffer_pointer = strtok(NULL, delim);
				sscanf(char_buffer_pointer, "%hu", &config->scale->notes);
				lingot_config_scale_allocate(config->scale,
						config->scale->notes);
				continue;
			}

			if (!strcmp(char_buffer_pointer, "NOTES")) {
				int i = 0;
				for (i = 0; i < config->scale->notes; i++) {
					line++;
					if (!fgets(char_buffer, max_line_size, fp))
						break;
					// tokens into the line.
					char_buffer_pointer = strtok(char_buffer, delim2);
					config->scale->note_name[i] = strdup(char_buffer_pointer);
					char_buffer_pointer = strtok(NULL, delim2);
					if (!lingot_config_scale_parse_shift(char_buffer_pointer,
							&config->scale->offset_cents[i],
							&config->scale->offset_ratios[0][i],
							&config->scale->offset_ratios[1][i])) {
						parse_errors = 1;
					}
				}
				line++;
				if (!fgets(char_buffer, max_line_size, fp))
					break; // }

				continue;
			}

			if (!strcmp(char_buffer_pointer, "}")) {
				reading_scale = 0;
				continue;
			}

		}

		deprecated_option = 0;
		for (option_index = 0; options[option_index]; option_index++) {
			if (!strcmp(char_buffer_pointer, options[option_index])) {
				break; // found token.
			} else if (!strcmp("|", options[option_index])) {
				deprecated_option = 1;
			}
		}

		option = options[option_index];
		param = params[option_index];

		if (!option) {
			fprintf(stderr,
					"warning: parse error at line %i: unknown keyword %s\n",
					line, char_buffer_pointer);
			parse_errors = 1;
			continue;
		}

		if (deprecated_option) {
			fprintf(stdout, "warning: deprecated option %s\n",
					char_buffer_pointer);
		}

		// take the attribute value.
		char_buffer_pointer = strtok(NULL, delim);

		if (!char_buffer_pointer) {
			fprintf(stderr, "warning: parse error at line %i: value expected\n",
					line);
			parse_errors = 1;
			continue;
		}

		// asign the value to the parameter.
		switch (option_formats[option_index]) {
		case 's':
			sprintf(((char*) param), "%s", char_buffer_pointer);
			command_count++;
			break;
		case 'd':
			sscanf(char_buffer_pointer, "%d", (unsigned int*) param);
			command_count++;
			break;
		case 'f':
			sscanf(char_buffer_pointer, "%f", &aux);
			*((FLT*) param) = aux;
			command_count++;
			break;
		case 'm':
			if (!strcmp("AUDIO_SYSTEM", option)) {
				command_count++;
				*((audio_system_t*) param) = str_to_audio_system_t(
						char_buffer_pointer);
				if (*((audio_system_t*) param) == (audio_system_t) -1) {
					*((audio_system_t*) param) = AUDIO_SYSTEM_ALSA;
					char buff[1000];
					sprintf(
							buff,
							_(
									"Error parsing the configuration file, line %i: unrecognized audio system '%s', assuming default audio system.\n"),
							line, char_buffer_pointer);

					lingot_msg_add_warning(buff);
					parse_errors = 1;
				}
			}
			break;
		}
	}

	fclose(fp);

	if (parse_errors) {
		lingot_msg_add_warning(
				_(
						"The configuration file contains errors, and hence some default values have been chosen. Consider checking the settings and fixing the problem using the configuration dialog."));
	}

	lingot_config_update_internal_params(config);
}

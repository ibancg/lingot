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

#define N_OPTIONS 21

LingotConfigParameterSpec parameters[N_OPTIONS];
unsigned int parameters_count = 0;

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

static void lingot_config_add_string_parameter_spec(LingotConfigParameterId id,
		const char* name, unsigned int max_len, int deprecated) {
	parameters[parameters_count].id = id;
	parameters[parameters_count].type = LINGOT_PARAMETER_TYPE_STRING;
	parameters[parameters_count].name = name;
	parameters[parameters_count].units = NULL;
	parameters[parameters_count].deprecated = deprecated;
	parameters[parameters_count].str_max_len = max_len;
	parameters_count++;
}

static void lingot_config_add_integer_parameter_spec(LingotConfigParameterId id,
		const char* name, const char* units, int min, int max, int deprecated) {
	parameters[parameters_count].id = id;
	parameters[parameters_count].type = LINGOT_PARAMETER_TYPE_INTEGER;
	parameters[parameters_count].name = name;
	parameters[parameters_count].units = units;
	parameters[parameters_count].deprecated = deprecated;
	parameters[parameters_count].int_min = min;
	parameters[parameters_count].int_max = max;
	parameters_count++;
}

static void lingot_config_add_double_parameter_spec(LingotConfigParameterId id,
		const char* name, const char* units, double min, double max,
		int deprecated) {
	parameters[parameters_count].id = id;
	parameters[parameters_count].type = LINGOT_PARAMETER_TYPE_FLOAT;
	parameters[parameters_count].name = name;
	parameters[parameters_count].units = units;
	parameters[parameters_count].deprecated = deprecated;
	parameters[parameters_count].float_min = min;
	parameters[parameters_count].float_max = max;
	parameters_count++;
}

void lingot_config_create_parameter_specs() {
	parameters[parameters_count].type = LINGOT_PARAMETER_ID_AUDIO_SYSTEM;
	parameters[parameters_count].type = LINGOT_PARAMETER_TYPE_AUDIO_SYSTEM;
	parameters[parameters_count].name = "AUDIO_SYSTEM";
	parameters[parameters_count].units = NULL;
	parameters[parameters_count].deprecated = 0;
	parameters_count++;

	lingot_config_add_string_parameter_spec(LINGOT_PARAMETER_ID_AUDIO_DEV,
			"AUDIO_DEV", 512, 0);
	lingot_config_add_string_parameter_spec(LINGOT_PARAMETER_ID_AUDIO_DEV_ALSA,
			"AUDIO_DEV_ALSA", 512, 0);
	lingot_config_add_string_parameter_spec(LINGOT_PARAMETER_ID_AUDIO_DEV_JACK,
			"AUDIO_DEV_JACK", 512, 0);
	lingot_config_add_string_parameter_spec(
			LINGOT_PARAMETER_ID_AUDIO_DEV_PULSEAUDIO, "AUDIO_DEV_PULSEAUDIO",
			512, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_SAMPLE_RATE,
			"SAMPLE_RATE", "Hz", 100, 200000, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_OVERSAMPLING,
			"OVERSAMPLING", NULL, 1, 120, 0);
	lingot_config_add_double_parameter_spec(
			LINGOT_PARAMETER_ID_ROOT_FREQUENCY_ERROR, "ROOT_FREQUENCY_ERROR",
			"cents", -500.0, 500.0, 0);
	lingot_config_add_double_parameter_spec(LINGOT_PARAMETER_ID_MIN_FREQUENCY,
			"MIN_FREQUENCY", "Hz", 0.0, 22050.0, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_FFT_SIZE,
			"FFT_SIZE", "samples", 256, 4096, 0);
	lingot_config_add_double_parameter_spec(LINGOT_PARAMETER_ID_TEMPORAL_WINDOW,
			"TEMPORAL_WINDOW", "seconds", 0.0, 15.00, 0);
	lingot_config_add_double_parameter_spec(LINGOT_PARAMETER_ID_NOISE_THRESHOLD,
			"NOISE_THRESHOLD", "dB", 0.0, 100.0, 0);
	lingot_config_add_double_parameter_spec(
			LINGOT_PARAMETER_ID_CALCULATION_RATE, "CALCULATION_RATE", "Hz", 1.0,
			30.00, 0);
	lingot_config_add_double_parameter_spec(
			LINGOT_PARAMETER_ID_VISUALIZATION_RATE, "VISUALIZATION_RATE", "Hz",
			1.0, 40.00, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_PEAK_NUMBER,
			"PEAK_NUMBER", "samples", 1, 10, 0);
	lingot_config_add_integer_parameter_spec(
			LINGOT_PARAMETER_ID_PEAK_HALF_WIDTH, "PEAK_HALF_WIDTH", "samples",
			1, 5, 0);
	lingot_config_add_double_parameter_spec(
			LINGOT_PARAMETER_ID_PEAK_REJECTION_RELATION,
			"PEAK_REJECTION_RELATION", "dB", 0.0, 100.0, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_DFT_NUMBER,
			"DFT_NUMBER", "DFTs", 0, 10, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_DFT_SIZE,
			"DFT_SIZE", "samples", 4, 100, 0);
	lingot_config_add_double_parameter_spec(LINGOT_PARAMETER_ID_GAIN, "GAIN",
			"dB", -90.0, 90.0, 0);
	lingot_config_add_integer_parameter_spec(LINGOT_PARAMETER_ID_PEAK_ORDER,
			"PEAK_ORDER", NULL, 0, 10, 1);
}

LingotConfigParameterSpec lingot_config_get_parameter_spec(
		LingotConfigParameterId id) {
	return parameters[(int) id];
}

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
}

//----------------------------------------------------------------------------

// internal parameters mapped to each token in the config file.
static void lingot_config_map_parameters(LingotConfig* config, void* params[]) {
	void* c_params[] = { //
			&config->audio_system, //
					&config->audio_dev[AUDIO_SYSTEM_OSS], //
					&config->audio_dev[AUDIO_SYSTEM_ALSA], //
					&config->audio_dev[AUDIO_SYSTEM_JACK], //
					&config->audio_dev[AUDIO_SYSTEM_PULSEAUDIO], //
					&config->sample_rate, //
					&config->oversampling, //
					&config->root_frequency_error, //
					&config->min_frequency, //
					&config->fft_size, //
					&config->temporal_window, //
					&config->noise_threshold_db, //
					&config->calculation_rate, //
					&config->visualization_rate, //
					&config->peak_number, //
					&config->peak_half_width, //
					&config->peak_rejection_relation_db, //
					&config->dft_number, //
					&config->dft_size, //
					&config->gain, //
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

	for (i = 0; i < N_OPTIONS; i++) {
		if (!parameters[i].deprecated) {

			//option = options[i];
			param = params[i];

			fprintf(fp, "%s = ", parameters[i].name);
			switch (parameters[i].type) {
			case LINGOT_PARAMETER_TYPE_STRING:
				fprintf(fp, "%s", (char*) param);
				break;
			case LINGOT_PARAMETER_TYPE_INTEGER:
				fprintf(fp, "%d", *((unsigned int*) param));
				break;
			case LINGOT_PARAMETER_TYPE_FLOAT:
				fprintf(fp, "%0.3f", *((FLT*) param));
				break;
			case LINGOT_PARAMETER_TYPE_AUDIO_SYSTEM:
				fprintf(fp, "%s",
						audio_system_t_to_str(*((audio_system_t*) param)));
				break;
			}

			if (parameters[i].units != NULL) {
				fprintf(fp, " # %s", parameters[i].units);
			}

			fprintf(fp, "\n");
		}
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
	int scale_errors = 0;
	LingotScale* scale = NULL;

	// restore default values for non specified parameters
	lingot_config_restore_default_values(config);

	lingot_config_map_parameters(config, params);

	static const unsigned int max_line_size = 1024;
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
			break;

		if (char_buffer[0] == '#')
			continue;

		// tokens into the line.
		char_buffer_pointer = strtok(char_buffer, delim);

		if (!char_buffer_pointer)
			continue; // blank line.

		if (!strcmp(char_buffer_pointer, "SCALE")) {
			reading_scale = 1;
			scale = lingot_config_scale_new();
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
				scale->name = strdup(char_buffer_pointer);
				continue;
			}
			if (!strcmp(char_buffer_pointer, "BASE_FREQUENCY")) {
				char_buffer_pointer = strtok(NULL, delim);
				sscanf(char_buffer_pointer, "%lg", &scale->base_frequency);
				continue;
			}
			if (!strcmp(char_buffer_pointer, "NOTE_COUNT")) {
				char_buffer_pointer = strtok(NULL, delim);
				sscanf(char_buffer_pointer, "%hu", &scale->notes);
				lingot_config_scale_allocate(scale, scale->notes);
				continue;
			}

			if (!strcmp(char_buffer_pointer, "NOTES")) {
				int i = 0;
				for (i = 0; i < scale->notes; i++) {
					line++;
					if (!fgets(char_buffer, max_line_size, fp)) {
						scale_errors = 1;
						fprintf(stderr,
								"error at line %i: error reading the scale\n",
								line);
						break;
					}

					// tokens into the line.
					char_buffer_pointer = strtok(char_buffer, delim2);

					if (char_buffer_pointer == NULL) {
						scale_errors = 1;
						fprintf(stderr,
								"error at line %i: error reading the scale\n",
								line);
						break;
					}

					scale->note_name[i] = strdup(char_buffer_pointer);
					char_buffer_pointer = strtok(NULL, delim2);

					if (char_buffer_pointer == NULL) {
						scale_errors = 1;
						fprintf(stderr,
								"error at line %i: error reading the scale\n",
								line);
						break;
					}

					if (!lingot_config_scale_parse_shift(char_buffer_pointer,
							&scale->offset_cents[i],
							&scale->offset_ratios[0][i],
							&scale->offset_ratios[1][i])) {
						scale_errors = 1;
					} else {

						if ((scale->offset_cents[i] < 0)
								|| (scale->offset_cents[i] >= 1200)) {
							scale_errors = 1;
							fprintf(
									stderr,
									"error at line %i: the notes in the scale must be equal or higher than 1/1 (0 cents) and lower than 2/1 (1200 cents)\n",
									line);
						}

						if (i == 0) {
							if (scale->offset_cents[i] != 0.0) {
								scale_errors = 1;
								fprintf(
										stderr,
										"error at line %i: the first note in the scale must be 1/1 (0 cents shift)\n",
										line);
							}
						} else if (scale->offset_cents[i]
								<= scale->offset_cents[i - 1]) {
							scale_errors = 1;
							fprintf(
									stderr,
									"error at line %i: the notes in the scale must be well ordered\n",
									line);
						}
					}
				}
				line++;
				if (!fgets(char_buffer, max_line_size, fp))
					break;

				continue;
			}

			if (!strcmp(char_buffer_pointer, "}")) {
				reading_scale = 0;
				continue;
			}

		}

		for (option_index = 0; option_index < N_OPTIONS; option_index++) {
			if (!strcmp(char_buffer_pointer, parameters[option_index].name)) {
				break; // found token.
			}
		}

		if (option_index == N_OPTIONS) {
			fprintf(stderr,
					"warning: parse error at line %i: unknown keyword %s\n",
					line, char_buffer_pointer);
			parse_errors = 1;
			continue;
		}

		if (parameters[option_index].deprecated) {
			fprintf(stdout, "warning: deprecated option %s\n",
					char_buffer_pointer);

		}

		param = params[option_index];

		// take the attribute value.
		char_buffer_pointer = strtok(NULL, delim);

		if (!char_buffer_pointer) {
			fprintf(stderr, "warning: parse error at line %i: value expected\n",
					line);
			parse_errors = 1;
			continue;
		}

		int int_value;
		float float_value;
		audio_system_t audio_system_value;

		// asign the value to the parameter.
		switch (parameters[option_index].type) {
		case LINGOT_PARAMETER_TYPE_STRING:
			if (strlen(char_buffer_pointer)
					< parameters[option_index].str_max_len) {
				sprintf(((char*) param), "%s", char_buffer_pointer);
			} else {
				fprintf(
						stderr,
						"error: parse error at line %i, '%s = %s': identifier too long (maximum length %d characters), assuming default value '%s'\n",
						line, parameters[option_index].name,
						char_buffer_pointer,
						parameters[option_index].str_max_len, ((char*) param));
				parse_errors = 1;
			}
			break;
		case LINGOT_PARAMETER_TYPE_INTEGER:
			sscanf(char_buffer_pointer, "%d", &int_value);

			if (parameters[option_index].id == LINGOT_PARAMETER_ID_FFT_SIZE) {
				if ((int_value != 256) && (int_value != 512)
						&& (int_value != 1024) && (int_value != 2048)
						&& (int_value != 4096)) {
					fprintf(
							stderr,
							"error: parse error at line %i, '%s = %s': invalid value (allowed values are 256, 512, 1024, 2048 and 4096), assuming default value %i\n",
							line, parameters[option_index].name,
							char_buffer_pointer, *((unsigned int*) param));
					parse_errors = 1;
				}
			} else {
				if ((int_value >= parameters[option_index].int_min)
						&& (int_value <= parameters[option_index].int_max)) {
					*((unsigned int*) param) = int_value;
				} else {
					fprintf(
							stderr,
							"error: parse error at line %i, '%s = %s': out of bounds (minimum %i, maximum %i), assuming default value %i\n",
							line, parameters[option_index].name,
							char_buffer_pointer,
							parameters[option_index].int_min,
							parameters[option_index].int_max,
							*((unsigned int*) param));
					parse_errors = 1;

				}
			}
			break;
		case LINGOT_PARAMETER_TYPE_FLOAT:
			sscanf(char_buffer_pointer, "%f", &float_value);
			if ((float_value >= parameters[option_index].float_min)
					&& (float_value <= parameters[option_index].float_max)) {
				*((FLT*) param) = float_value;
			} else {
				fprintf(
						stderr,
						"error: parse error at line %i, '%s = %s': out of bounds (minimum %0.3f, maximum %0.3f), assuming default value %0.3f\n",
						line, parameters[option_index].name,
						char_buffer_pointer, parameters[option_index].float_min,
						parameters[option_index].float_max, *((FLT*) param));
				parse_errors = 1;
			}
			break;
		case LINGOT_PARAMETER_TYPE_AUDIO_SYSTEM:
			audio_system_value = str_to_audio_system_t(char_buffer_pointer);
			if (audio_system_value != (audio_system_t) -1) {
				*((audio_system_t*) param) = audio_system_value;
			} else {
				char buff[1000];
				sprintf(
						buff,
						_(
								"Error parsing the configuration file, line %i: unrecognized audio system, assuming default value.\n"),
						line);

				lingot_msg_add_warning(buff);
				parse_errors = 1;
			}
			break;
		}
	}

	fclose(fp);

	if (scale != NULL) {
		if (!scale_errors) {
			config->scale = lingot_config_scale_new();
			lingot_config_scale_copy(config->scale, scale);
		}

		lingot_config_scale_destroy(scale);
		free(scale);
	}

	if (parse_errors || scale_errors) {
		lingot_msg_add_warning(
				_(
						"The configuration file contains errors, and hence some default values have been chosen. The problem will be fixed once you have accepted the settings in the configuration dialog."));
	}

	lingot_config_update_internal_params(config);
}

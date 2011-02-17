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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lingot-config-scale.h"

LingotScale* lingot_config_scale_new() {

	LingotScale* scale = malloc(sizeof(LingotScale));

	scale->name = NULL;
	scale->notes = 0;
	scale->note_name = NULL;
	scale->offset_cents = NULL;
	scale->offset_ratios[0] = NULL;
	scale->offset_ratios[1] = NULL;
	scale->base_frequency = 0.0;

	return scale;
}

void lingot_config_scale_allocate(LingotScale* scale, unsigned short int notes) {
	scale->notes = notes;
	scale->note_name = malloc(notes * sizeof(char*));
	scale->offset_cents = malloc(notes * sizeof(FLT));
	scale->offset_ratios[0] = malloc(notes * sizeof(short int));
	scale->offset_ratios[1] = malloc(notes * sizeof(short int));
}

void lingot_config_scale_destroy(LingotScale* scale) {
	unsigned short int i;
	for (i = 0; i < scale->notes; i++) {
		free(scale->note_name[i]);
	}

	if (scale->offset_cents != NULL)
		free(scale->offset_cents);

	if (scale->offset_ratios[0] != NULL)
		free(scale->offset_ratios[0]);
	if (scale->offset_ratios[1] != NULL)
		free(scale->offset_ratios[1]);

	if (scale->note_name != NULL)
		free(scale->note_name);
	if (scale->name != NULL)
		free(scale->name);

	scale->name = NULL;
	scale->notes = 0;
	scale->note_name = NULL;
	scale->offset_cents = NULL;
	scale->offset_ratios[0] = NULL;
	scale->offset_ratios[1] = NULL;
	scale->base_frequency = 0.0;
}

void lingot_config_scale_restore_default_values(LingotScale* scale) {

	unsigned short int i;
	static char* tone_string[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G",
			"G#", "A", "A#", "B", };

	lingot_config_scale_destroy(scale);

	// default 12 tones equal-tempered scale hard-coded
	scale->name = strdup("Default equal-tempered scale");
	lingot_config_scale_allocate(scale, 12);

	scale->base_frequency = MID_C_FREQUENCY;

	scale->note_name[0] = strdup(tone_string[0]);
	scale->offset_cents[0] = 0.0;
	scale->offset_ratios[0][0] = 1;
	scale->offset_ratios[1][0] = 1; // 1/1

	for (i = 1; i < scale->notes; i++) {
		scale->note_name[i] = strdup(tone_string[i]);
		scale->offset_cents[i] = 100.0 * i;
		scale->offset_ratios[0][i] = -1; // not used
		scale->offset_ratios[1][i] = -1; // not used
	}
}

void lingot_config_scale_copy(LingotScale* dst, LingotScale* src) {
	unsigned short int i;

	lingot_config_scale_destroy(dst);

	*dst = *src;

	dst->name = strdup(src->name);
	lingot_config_scale_allocate(dst, dst->notes);

	for (i = 0; i < dst->notes; i++) {
		dst->note_name[i] = strdup(src->note_name[i]);
		dst->offset_cents[i] = src->offset_cents[i];
		dst->offset_ratios[0][i] = src->offset_ratios[0][i];
		dst->offset_ratios[1][i] = src->offset_ratios[1][i];
	}
}

void lingot_config_scale_parse_shift(char* char_buffer, double* cents,
		short int* numerator, short int* denominator) {
	const static char* delim = "/";
	char* char_buffer_pointer1 = strtok(char_buffer, delim);
	char* char_buffer_pointer2 = strtok(NULL, delim);
	short int num, den;

	if (numerator != NULL) {
		*numerator = -1;
	}

	if (denominator != NULL) {
		*denominator = -1;
	}

	if (!char_buffer_pointer2) {
		sscanf(char_buffer_pointer1, "%lf", cents);
	} else {
		sscanf(char_buffer_pointer1, "%hd", &num);
		sscanf(char_buffer_pointer2, "%hd", &den);
		*cents = 1200.0 * log2(1.0 * num / den);
		if (numerator != NULL) {
			*numerator = num;
		}
		if (denominator != NULL) {
			*denominator = den;
		}
	}
}

void lingot_config_scale_format_shift(char* char_buffer, double cents,
		short int numerator, short int denominator) {
	if (numerator < 0) {
		sprintf(char_buffer, "%0.4lf", cents);
	} else {
		sprintf(char_buffer, "%hd/%hd", numerator, denominator);
	}
}

int lingot_config_scale_load(LingotScale* scale, char* filename) {
	FILE* fp;
	int i;
	char* char_buffer_pointer1;
	char* nl;
	const static char* delim = " \t\n";

#   define MAX_LINE_SIZE 1000

	char char_buffer[MAX_LINE_SIZE];

	if ((fp = fopen(filename, "r")) == NULL) {
		sprintf(char_buffer, "error opening scale file %s", filename);
		perror(char_buffer);
		return 0;
	}

	scale->base_frequency = MID_C_FREQUENCY;

	fgets(char_buffer, MAX_LINE_SIZE, fp);
	if (strchr(char_buffer, '!') != char_buffer) {
		fclose(fp);
		return 0;
	}

	fgets(char_buffer, MAX_LINE_SIZE, fp);
	fgets(char_buffer, MAX_LINE_SIZE, fp);

	nl = strrchr(char_buffer, '\r');
	if (nl)
		*nl = '\0';
	nl = strrchr(char_buffer, '\n');
	if (nl)
		*nl = '\0';
	scale->name = strdup(char_buffer);

	fgets(char_buffer, MAX_LINE_SIZE, fp);
	sscanf(char_buffer, "%hu", &scale->notes);

	fgets(char_buffer, MAX_LINE_SIZE, fp);
	lingot_config_scale_allocate(scale, scale->notes);

	scale->note_name[0] = strdup("1");
	scale->offset_cents[0] = 0.0;
	scale->offset_ratios[0][0] = 1;
	scale->offset_ratios[1][0] = 1; // 1/1

	for (i = 1; i < scale->notes; i++) {

		fgets(char_buffer, MAX_LINE_SIZE, fp);

		char_buffer_pointer1 = strtok(char_buffer, delim);

		lingot_config_scale_parse_shift(char_buffer_pointer1,
				&scale->offset_cents[i], &scale->offset_ratios[0][i],
				&scale->offset_ratios[1][i]);
		//scale->note_freq_ratio[i] = pow(2.0, scale->offset_cents[i] / 1200.0);
		//		if (strstr(char_buffer_pointer1, "/") != NULL) {
		//			scale->note_freq_ratio_str[i] = strdup(char_buffer_pointer1);
		//		} else {
		//			sprintf(char_buffer, "%f", scale->note_freq_ratio[i]);
		//			scale->note_freq_ratio_str[i] = strdup(char_buffer);
		//		}

		sprintf(char_buffer, "%d", i + 1);
		scale->note_name[i] = strdup(char_buffer);
	}

	fclose(fp);

#   undef MAX_LINE_SIZE
	return 1;
}
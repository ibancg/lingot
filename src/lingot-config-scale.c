/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña.
 * Copyright (C) 2004-2008  Jairo Chapela Martínez.

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
#include <errno.h>
#include <string.h>
#include <math.h>

#include "lingot-config-scale.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

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
	unsigned int i = 0;
	for (i = 0; i < notes; i++) {
		scale->note_name[i] = 0x0;
	}
	scale->offset_cents = malloc(notes * sizeof(FLT));
	scale->offset_ratios[0] = malloc(notes * sizeof(short int));
	scale->offset_ratios[1] = malloc(notes * sizeof(short int));
}

void lingot_config_scale_destroy(LingotScale* scale) {
	unsigned short int i;

	if (scale != NULL) {
		for (i = 0; i < scale->notes; i++)
			if (scale->note_name[i] != 0x0) {
				free(scale->note_name[i]);
			}

		if (scale->offset_cents != NULL) {
			free(scale->offset_cents);
		}

		if (scale->offset_ratios[0] != NULL) {
			free(scale->offset_ratios[0]);
		}
		if (scale->offset_ratios[1] != NULL) {
			free(scale->offset_ratios[1]);
		}

		if (scale->note_name != NULL) {
			free(scale->note_name);
		}
		if (scale->name != NULL) {
			free(scale->name);
		}

		scale->name = NULL;
		scale->notes = 0;
		scale->note_name = NULL;
		scale->offset_cents = NULL;
		scale->offset_ratios[0] = NULL;
		scale->offset_ratios[1] = NULL;
		scale->base_frequency = 0.0;
	}
}

void lingot_config_scale_restore_default_values(LingotScale* scale) {

	unsigned short int i;
	// TODO: i18n
	const char* tone_string[] = { _("C"), _("C#"), _("D"), _("D#"), _("E"), _("F"), _(
			"F#"), _("G"), _("G#"), _("A"), _("A#"), _("B"), };

	lingot_config_scale_destroy(scale);

	// default 12 tones equal-tempered scale hard-coded
	scale->name = strdup(_("Default equal-tempered scale"));
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

int lingot_config_scale_get_octave(const LingotScale* scale, int index) {
	int result = 0;
	if (index < 0) {
		result = ((index + 1) / scale->notes) - 1;
	} else {
		result = index / scale->notes;
	}
	return result;
}

int lingot_config_scale_get_note_index(const LingotScale* scale, int index) {
	int index2 = index % scale->notes;
	if (index2 < 0) {
		index2 += scale->notes;
	}
	return index2;
}

FLT lingot_config_scale_get_absolute_offset(const LingotScale* scale, int index) {
	return lingot_config_scale_get_octave(scale, index) * 1200.0
			+ scale->offset_cents[lingot_config_scale_get_note_index(scale,
					index)];
}

FLT lingot_config_scale_get_frequency(const LingotScale* scale, int index) {
	return scale->base_frequency
			* pow(2.0,
					lingot_config_scale_get_absolute_offset(scale, index)
							/ 1200.0);
}

// TODO: test
int lingot_config_scale_get_closest_note_index(const LingotScale* scale,
FLT freq, FLT deviation, FLT* error_cents) {

	short note_index = 0;
	short int index;

	FLT offset = 1200.0 * log2(freq / scale->base_frequency) - deviation;
	int octave = 0;
	octave = floor(offset / 1200);
	offset = fmod(offset, 1200.0);
	if (offset < 0.0) {
		offset += 1200.0;
	}

	index = floor(scale->notes * offset / 1200.0);

	// TODO: bisection?, avoid loop?
	FLT pitch_inf;
	FLT pitch_sup;
	int n = 0;
	for (;;) {
		n++;
		pitch_inf = scale->offset_cents[index];
		pitch_sup =
				((index + 1) < scale->notes) ?
						scale->offset_cents[index + 1] : 1200.0;

		if (offset > pitch_sup) {
			index++;
			continue;
		}

		if (offset < pitch_inf) {
			index--;
			continue;
		}

		break;
	};

	if (fabs(offset - pitch_inf) < fabs(offset - pitch_sup)) {
		note_index = index;
		*error_cents = offset - pitch_inf;
	} else {
		note_index = index + 1;
		*error_cents = offset - pitch_sup;
	}

	if (note_index == scale->notes) {
		note_index = 0;
		octave++;
	}

	note_index += octave * scale->notes;

	return note_index;
}

int lingot_config_scale_parse_shift(char* char_buffer, double* cents,
		short int* numerator, short int* denominator) {
	const static char* delim = "/";
	char* char_buffer_pointer1 = strtok(char_buffer, delim);
	char* char_buffer_pointer2 = strtok(NULL, delim);
	short int num, den;
	int result = 1;

	if (numerator != NULL) {
		*numerator = -1;
	}

	if (denominator != NULL) {
		*denominator = -1;
	}

	int n = 0;
	if (!char_buffer_pointer2) {
		if (!char_buffer_pointer1) {
			result = 0;
		} else {
			n = sscanf(char_buffer_pointer1, "%lf", cents);
			if (!n) {
				result = 0;
			}
		}
	} else {
		n = sscanf(char_buffer_pointer1, "%hd", &num);
		if (!n) {
			result = 0;
		} else {
			n = sscanf(char_buffer_pointer2, "%hd", &den);
			if (!n) {
				result = 0;
			} else {
				*cents = 1200.0 * log2(1.0 * num / den);
				if (numerator != NULL) {
					*numerator = num;
				}
				if (denominator != NULL) {
					*denominator = den;
				}
			}
		}
	}

	if (!result) {
		*numerator = 1;
		*denominator = 1;
		*cents = 0.0;
	}

	return result;
}

void lingot_config_scale_format_shift(char* char_buffer, double cents,
		short int numerator, short int denominator) {
	if (numerator < 0) {
		sprintf(char_buffer, "%0.4lf", cents);
	} else {
		sprintf(char_buffer, "%hd/%hd", numerator, denominator);
	}
}

int lingot_config_scale_load_scl(LingotScale* scale, char* filename) {
	FILE* fp;
	int i;
	char* char_buffer_pointer1;
	char* nl;
	const static char* delim = " \t\n";
	int result = 1;
	int line = 0;
	const char* error_format_msg = _("incorrect format");
	const char* error_note_number_msg = _("note number mismatch");
	const char* exception;

#   define MAX_LINE_SIZE 1000

	char char_buffer[MAX_LINE_SIZE];

	if ((fp = fopen(filename, "r")) == NULL) {
		char buff[1000];
		snprintf(buff, sizeof(buff), "%s\n%s", _("Error opening scale file."),
				strerror(errno));
		lingot_msg_add_error(buff);
		return 0;
	}

	scale->base_frequency = MID_C_FREQUENCY;

	try
	{
		line++;
		if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
			throw(error_format_msg);
		}

		if (strchr(char_buffer, '!') != char_buffer) {
			throw(error_format_msg);
		}

		line++;
		if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
			throw(error_format_msg);
		}

		line++;
		if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
			throw(error_format_msg);
		}

		nl = strrchr(char_buffer, '\r');
		if (nl)
			*nl = '\0';
		nl = strrchr(char_buffer, '\n');
		if (nl)
			*nl = '\0';
		scale->name = strdup(char_buffer);

		line++;
		if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
			throw(error_format_msg);
		}
		sscanf(char_buffer, "%hu", &scale->notes);

		line++;
		if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
			throw(error_format_msg);
		}
		lingot_config_scale_allocate(scale, scale->notes);

		scale->note_name[0] = strdup("1");
		scale->offset_cents[0] = 0.0;
		scale->offset_ratios[0][0] = 1;
		scale->offset_ratios[1][0] = 1; // 1/1

		for (i = 1; i < scale->notes; i++) {

			line++;
			if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
				throw(error_note_number_msg);
			}

			char_buffer_pointer1 = strtok(char_buffer, delim);

			if (char_buffer_pointer1 == NULL) {
				throw(error_note_number_msg);
			}

			int r = lingot_config_scale_parse_shift(char_buffer_pointer1,
					&scale->offset_cents[i], &scale->offset_ratios[0][i],
					&scale->offset_ratios[1][i]);
			if (!r) {
				throw(error_format_msg);
			} else {
				if (scale->offset_cents[i] <= scale->offset_cents[i - 1]) {
					throw(_("the notes must be well ordered"));
				}
			}

			sprintf(char_buffer, "%d", i + 1);
			scale->note_name[i] = strdup(char_buffer);
		}
	}catch {
		result = 0;
		char buff[1000];
		snprintf(buff, sizeof(buff), "%s, line %i: %s",
				_("Error opening scale file"), line, exception);
		lingot_msg_add_error(buff);
	}

	fclose(fp);

#   undef MAX_LINE_SIZE
	return result;
}

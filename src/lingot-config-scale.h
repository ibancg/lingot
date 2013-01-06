/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef LINGOT_CONFIG_SCALE_H_
#define LINGOT_CONFIG_SCALE_H_

#include "lingot-defs.h"

typedef struct _LingotScale LingotScale;

struct _LingotScale {
	char* name; // name of the scale
	unsigned short int notes; // number of notes
	FLT* offset_cents; // offset in cents
	short int* offset_ratios[2]; // offset in ratios (pairs of integers)
	FLT base_frequency; // frequency of the first note
	char** note_name; // note names

	// -- internal parameters --

	FLT max_offset_rounded; // round version of maximum offset in cents
};

LingotScale* lingot_config_scale_new();
void lingot_config_scale_allocate(LingotScale* scale, unsigned short int notes);
void lingot_config_scale_destroy(LingotScale* scale);
int lingot_config_scale_load_scl(LingotScale* scale, char* filename);
int lingot_config_scale_parse_shift(char*, double*, short int*, short int*);
void lingot_config_scale_format_shift(char*, double, short int, short int);
void lingot_config_scale_copy(LingotScale* dst, LingotScale* src);
void lingot_config_scale_restore_default_values(LingotScale* scale);

#endif /* LINGOT_CONFIG_SCALE_H_ */

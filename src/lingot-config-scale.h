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

#ifndef LINGOT_CONFIG_SCALE_H_
#define LINGOT_CONFIG_SCALE_H_

#include "lingot-defs.h"

typedef struct {
	char* name; 					// name of the scale
	unsigned short int notes; 		// number of notes
	FLT* offset_cents; 				// offset in cents
	short int* offset_ratios[2]; 	// offset in ratios (pairs of integers)
	FLT base_frequency; 			// frequency of the first note (tipically C4)
	char** note_name; 				// note names

	// -- internal parameters --

	FLT max_offset_rounded; 		// round version of maximum offset in cents
} LingotScale;

void lingot_config_scale_new(LingotScale*);
void lingot_config_scale_allocate(LingotScale* scale, unsigned short int notes);
void lingot_config_scale_destroy(LingotScale* scale);

int lingot_config_scale_load_scl(LingotScale* scale, char* filename);
/* The scale argument must *not* be initialized with _new. */

int lingot_config_scale_parse_shift(char*, double*, short int*, short int*);

void lingot_config_scale_format_shift(char*, double, short int, short int);

void lingot_config_scale_copy(LingotScale* dst, const LingotScale* src);
/* The dst argument *must* be initialized with _new. */

void lingot_config_scale_restore_default_values(LingotScale* scale);

// Gets the note index within the range [0, num notes)
int lingot_config_scale_get_note_index(const LingotScale* scale, int index);

// Gets an octave number from the note index
// Octave 0 starts at index 0
int lingot_config_scale_get_octave(const LingotScale* scale, int index);

// Gets the frequency of a given note in the scale.
// The final absolute frequency depends on the base_frequency configured for
// the scale, which corresponds to note index 0.
// Tipically, the base frequency is C4, so in order to get C1 in a 12-note scale,
// we need to pass -36 here as note index
FLT lingot_config_scale_get_frequency(const LingotScale* scale, int index);


int lingot_config_scale_get_closest_note_index(const LingotScale* scale,
		FLT freq, FLT deviation, FLT* error_cents);

#endif /* LINGOT_CONFIG_SCALE_H_ */

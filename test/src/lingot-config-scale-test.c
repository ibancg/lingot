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

void lingot_config_scale_test() {

	initTestCase("lingot_config_scale_test");

	lingot_config_create_parameter_specs();
	LingotConfig* config = lingot_config_new();
	lingot_config_restore_default_values(config);
	LingotScale* scale = config->scale;

	double error_cents;
	int note_index = 0;
	int closest_note_index = 0;
	closest_note_index = lingot_config_scale_get_closest_note_index(scale,
			130.812782, 0.0, &error_cents);
	printf("closest note %i\n", closest_note_index);
	assert(closest_note_index == -12);
	closest_note_index = lingot_config_scale_get_closest_note_index(scale,
			130.81, 0.0, &error_cents);
	assert(closest_note_index == -12);

	lingot_config_destroy(config);

	finishTestCase();
}

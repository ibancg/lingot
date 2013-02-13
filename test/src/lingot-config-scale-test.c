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

	lingot_config_create_parameter_specs();
	LingotConfig* config = lingot_config_new();
	lingot_config_restore_default_values(config);
	LingotScale* scale = config->scale;

	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 0), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 1), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 10), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 11), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 12), 1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 23), 1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, 24), 2);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -1), -1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -10), -1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -11), -1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -12), -1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -13), -2);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -23), -2);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -24), -2);
	CU_ASSERT_EQUAL(lingot_config_scale_get_octave(scale, -25), -3);

	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 0), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 1), 1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 10), 10);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 11), 11);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 12), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 23), 11);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, 24), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -1), 11);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -2), 10);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -11), 1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -12), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -23), 1);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -24), 0);
	CU_ASSERT_EQUAL(lingot_config_scale_get_note_index(scale, -25), 11);

	double error_cents;
	int closest_note_index = 0;

	closest_note_index = lingot_config_scale_get_closest_note_index(scale,
			140.812782, 0.0, &error_cents);
	CU_ASSERT_EQUAL(closest_note_index, -11);
	closest_note_index = lingot_config_scale_get_closest_note_index(scale,
			130.812782, 0.0, &error_cents);
	CU_ASSERT_EQUAL(closest_note_index, -12);
	closest_note_index = lingot_config_scale_get_closest_note_index(scale,
			130.81, 0.0, &error_cents);
	CU_ASSERT_EQUAL(closest_note_index, -12);
	closest_note_index = lingot_config_scale_get_closest_note_index(scale,
			130.82, 0.0, &error_cents);
	CU_ASSERT_EQUAL(closest_note_index, -12);

	lingot_config_destroy(config);
}

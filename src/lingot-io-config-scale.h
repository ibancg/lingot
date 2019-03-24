/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2019  Iban Cereijo.
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

#ifndef LINGOT_IO_CONFIG_SCALE_H
#define LINGOT_IO_CONFIG_SCALE_H

#include "lingot-config-scale.h"

int lingot_config_scale_load_scl(LingotScale* scale, char* filename);
/* The scale argument must *not* be initialized with _new. */

int lingot_config_scale_parse_shift(char*, double*, short int*, short int*);

void lingot_config_scale_format_shift(char*, double, short int, short int);

#endif // LINGOT_IO_CONFIG_SCALE_H

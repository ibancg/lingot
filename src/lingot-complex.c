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

#include "lingot-complex.h"

/**
 * addition. All parameters are overlapables.
 */
void lingot_complex_add(LingotComplex* a, LingotComplex* b, LingotComplex* c) {
	c->r = a->r + b->r;
	c->i = a->i + b->i;
}

/**
 * substraction. All parameters are overlapables.
 */
void lingot_complex_sub(LingotComplex* a, LingotComplex* b, LingotComplex* c) {
	c->r = a->r - b->r;
	c->i = a->i - b->i;
}

/**
 * multiplication. All parameters are overlapables.
 */
void lingot_complex_mul(LingotComplex* a, LingotComplex* b, LingotComplex* c) {
	if ((c != a) && (c != b)) {
		c->r = a->r * b->r - a->i * b->i;
		c->i = a->i * b->r + a->r * b->i;
	} else {
		LingotComplex r;
		r.r = a->r * b->r - a->i * b->i;
		r.i = a->i * b->r + a->r * b->i;
		*c = r;
	}
}

/**
 * division. All parameters are overlapables.
 */
void lingot_complex_div(LingotComplex* a, LingotComplex* b, LingotComplex* c) {
	FLT bm2 = b->r * b->r + b->i * b->i;

	if ((c != a) && (c != b)) {
		c->r = (a->r * b->r + a->i * b->i) / bm2;
		c->i = (a->i * b->r - a->r * b->i) / bm2;
	} else {
		LingotComplex r;
		r.r = (a->r * b->r + a->i * b->i) / bm2;
		r.i = (a->i * b->r - a->r * b->i) / bm2;
		*c = r;
	}
}

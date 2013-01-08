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

#include "lingot-complex.h"

void lingot_complex_add(const LingotComplex a, const LingotComplex b,
		LingotComplex c) {
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
}

void lingot_complex_sub(const LingotComplex a, const LingotComplex b,
		LingotComplex c) {
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
}

void lingot_complex_mul(const LingotComplex a, const LingotComplex b,
		LingotComplex c) {
	c[0] = a[0] * b[0] - a[1] * b[1];
	c[1] = a[1] * b[0] + a[0] * b[1];
}

void lingot_complex_div(const LingotComplex a, const LingotComplex b,
		LingotComplex c) {
	FLT bm2 = b[0] * b[0] + b[1] * b[1];
	c[0] = (a[0] * b[0] + a[1] * b[1]) / bm2;
	c[1] = (a[1] * b[0] - a[0] * b[1]) / bm2;
}

void lingot_complex_mul_by(LingotComplex a, const LingotComplex b) {
	double rr = a[0] * b[0] - a[1] * b[1];
	a[1] = a[1] * b[0] + a[0] * b[1];
	a[0] = rr;
}

void lingot_complex_div_by(LingotComplex a, const LingotComplex b) {
	FLT bm2 = b[0] * b[0] + b[1] * b[1];
	double rr = (a[0] * b[0] + a[1] * b[1]) / bm2;
	a[1] = (a[1] * b[0] - a[0] * b[1]) / bm2;
	a[0] = rr;
}

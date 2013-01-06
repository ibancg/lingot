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

#include <memory.h>
#include <math.h>

#include "lingot-complex.h"
#include "lingot-filter.h"

#define max(a,b) (((a)<(b))?(b):(a))

// given each polynomial order and coefs, with optional initial status.
LingotFilter* lingot_filter_new(unsigned int Na, unsigned int Nb, FLT* a,
		FLT* b) {
	unsigned int i;
	LingotFilter* filter = malloc(sizeof(LingotFilter));
	filter->N = max(Na, Nb);

	filter->a = malloc((filter->N + 1) * sizeof(FLT));
	filter->b = malloc((filter->N + 1) * sizeof(FLT));
	filter->s = malloc((filter->N + 1) * sizeof(FLT));

	for (i = 0; i < filter->N + 1; i++)
		filter->a[i] = filter->b[i] = filter->s[i] = 0.0;

	memcpy(filter->a, a, (Na + 1) * sizeof(FLT));
	memcpy(filter->b, b, (Nb + 1) * sizeof(FLT));

	for (i = 0; i < filter->N + 1; i++) {
		filter->a[i] /= a[0]; // polynomial normalization.
		filter->b[i] /= a[0];
	}

	return filter;
}

void lingot_filter_destroy(LingotFilter* filter) {
	free(filter->a);
	free(filter->b);
	free(filter->s);

	free(filter);
}

// Digital Filter Implementation II, in & out overlapables.
void lingot_filter_filter(LingotFilter* filter, unsigned int n, FLT* in,
		FLT* out) {
	FLT w, y;
	register unsigned int i;
	register int j;

	for (i = 0; i < n; i++) {

		w = in[i];
		y = 0.0;

		for (j = filter->N - 1; j >= 0; j--) {
			w -= filter->a[j + 1] * filter->s[j];
			y += filter->b[j + 1] * filter->s[j];
			filter->s[j + 1] = filter->s[j];
		}

		y += w * filter->b[0];
		filter->s[0] = w;

		out[i] = y;
	}
}

// single sample filtering
FLT lingot_filter_filter_sample(LingotFilter* filter, FLT in) {
	FLT result;

	lingot_filter_filter(filter, 1, &in, &result);
	return result;
}

// vector prod
void lingot_filter_vector_product(int n, LingotComplex* vector,
		LingotComplex* result) {
	register int i;
	LingotComplex aux1;

	result->r = 1.0;
	result->i = 0.0;

	for (i = 0; i < n; i++) {
		aux1.r = -vector[i].r;
		aux1.i = -vector[i].i;
		lingot_complex_mul_by(result, &aux1);
	}

}

// Chebyshev filters
LingotFilter* lingot_filter_cheby_design(unsigned int n, FLT Rp, FLT wc) {
	int i; // loops
	int k;
	int p;

	FLT a[n + 1];
	FLT b[n + 1];

	FLT new_a[n + 1];
	FLT new_b[n + 1];

	// locate poles
	LingotComplex pole[n];

	for (i = 0; i < n; i++) {
		pole[i].r = 0.0;
		pole[i].i = 0.0;
	}

	FLT T = 2.0;
	// 2Hz
	FLT W = 2.0 / T * tan(M_PI * wc / T);

	FLT epsilon = sqrt(pow(10.0, 0.1 * Rp) - 1);
	FLT v0 = asinh(1 / epsilon) / n;

	FLT sv0 = sinh(v0);
	FLT cv0 = cosh(v0);

	FLT t;

	for (i = -(n - 1), k = 0; k < n; i += 2, k++) {
		t = M_PI * i / (2.0 * n);
		pole[k].r = -sv0 * cos(t);
		pole[k].i = cv0 * sin(t);
	}

	LingotComplex gain;

	lingot_filter_vector_product(n, pole, &gain);

	if ((n & 1) == 0) {// even
		FLT f = pow(10.0, -0.05 * Rp);
		gain.r *= f;
		gain.i *= f;
	}

	FLT f = pow(W, n);
	gain.r *= f;
	gain.i *= f;

	for (i = 0; i < n; i++) {
		pole[i].r *= W;
		pole[i].i *= W;
	}

	// bilinear transform
	LingotComplex sp[n];

	for (i = 0; i < n; i++) {
		sp[i].r = (2.0 - pole[i].r * T) / T;
		sp[i].i = (0.0 - pole[i].i * T) / T;
	}

	LingotComplex tmp1;
	LingotComplex aux2;

	lingot_filter_vector_product(n, sp, &tmp1);

	lingot_complex_div_by(&gain, &tmp1);

	for (i = 0; i < n; i++) {
		tmp1.r = (2.0 + pole[i].r * T);
		tmp1.i = (0.0 + pole[i].i * T);
		aux2.r = (2.0 - pole[i].r * T);
		aux2.i = (0.0 - pole[i].i * T);
		lingot_complex_div(&tmp1, &aux2, &pole[i]);
	}

	// compute filter coefficients from pole/zero values
	a[0] = 1.0;
	b[0] = 1.0;
	new_a[0] = 1.0;
	new_b[0] = 1.0;

	for (i = 1; i <= n; i++) {
		a[i] = 0.0;
		b[i] = 0.0;
		new_a[i] = 0.0;
		new_b[i] = 0.0;
	}

	if ((n & 1) == 1) // odd
	{
		// first subfilter is first order
		a[1] = -pole[n / 2].r;
		b[1] = 1.0;
	}

	// iterate over the conjugate pairs
	for (p = 0; p < n / 2; p++) {
		FLT b1 = 2.0;
		FLT b2 = 1.0;

		FLT a1 = -2.0 * pole[p].r;
		FLT a2 = pole[p].r * pole[p].r + pole[p].i * pole[p].i;

		// 2nd order subfilter per each pair
		new_a[1] = a[1] + a1 * a[0];
		new_b[1] = b[1] + b1 * b[0];

		// poly multiplication
		for (i = 2; i <= n; i++) {
			new_a[i] = a[i] + a1 * a[i - 1] + a2 * a[i - 2];
			new_b[i] = b[i] + b1 * b[i - 1] + b2 * b[i - 2];
		}
		for (i = 1; i <= n; i++) {
			a[i] = new_a[i];
			b[i] = new_b[i];
		}
	}

	gain.r = fabs(gain.r);
	for (i = 0; i <= n; i++) {
		b[i] *= gain.r;
	}

	return lingot_filter_new(n, n, a, b);
}

/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2013-2020  Iban Cereijo
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

#include <stdio.h>
#include <sys/time.h>
#include "lingot-test.h"

static struct timeval tv1;
static struct timeval tv2;

void tic(void) {
    gettimeofday(&tv1, NULL);
}

double toc(void) {

    double result = 0.0;

    gettimeofday(&tv2, NULL);

    result = ((double) (tv2.tv_sec - tv1.tv_sec)) + 1e-6 * ((double) (tv2.tv_usec - tv1.tv_usec));

    printf("Elapsed time: %g s.\n", result);
    return result;
}

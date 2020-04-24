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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "lingot-test.h"
#include "lingot-filter.h"
#include "lingot-signal.h"

void lingot_test_signal(void) {

    // -------------------------------------------------------------------------------------------

    LINGOT_FLT multiplier1 = 0.0;
    LINGOT_FLT multiplier2 = 0.0;

    int rel = lingot_signal_frequencies_related(100.0, 150.001, 20.0, &multiplier1,
                                                &multiplier2);
    CU_ASSERT_EQUAL(rel, 1);
    CU_ASSERT_EQUAL(multiplier1, 0.5);

    rel = lingot_signal_frequencies_related(100.0, 200.01, 20.0, &multiplier1,
                                            &multiplier2);
    CU_ASSERT_EQUAL(rel, 1);
    CU_ASSERT_EQUAL(multiplier1, 1.0);

    rel = lingot_signal_frequencies_related(200.0, 100.01, 20.0, &multiplier1,
                                            &multiplier2);
    CU_ASSERT_EQUAL(rel, 1);
    CU_ASSERT_EQUAL(multiplier1, 0.5);

    rel = lingot_signal_frequencies_related(100.0, 150.001, 70.0, &multiplier1,
                                            &multiplier2);
    CU_ASSERT_EQUAL(rel, 0);

    rel = lingot_signal_frequencies_related(22.788177, 114.008917, 15.0,
                                            &multiplier1, &multiplier2);
    CU_ASSERT_EQUAL(rel, 1);
    CU_ASSERT_EQUAL(multiplier1, 1.0);
    CU_ASSERT_EQUAL(multiplier2, 0.2);


    rel = lingot_signal_frequencies_related(97.959328, 48.977020, 15.0,
                                            &multiplier1, &multiplier2);
    CU_ASSERT_EQUAL(rel, 1);
    CU_ASSERT_EQUAL(multiplier1, 0.5);
    CU_ASSERT_EQUAL(multiplier2, 1.0);

    // -------------------------------------------------------------------------------------------

    int N = 16;
    int i = 0;
    int n = 5;
    LINGOT_FLT* spd = malloc((size_t) N * sizeof(LINGOT_FLT));
    LINGOT_FLT* noise = malloc((size_t) N * sizeof(LINGOT_FLT));

    for (i = 0; i < N; i++) {
        spd[i] = i + 1;
        noise[i] = -1.0;
    }

    lingot_signal_compute_noise_level(spd, N, n, noise);

    // TODO: enable logic

    //	printf("S = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", spd[i]);
    //	}
    //	printf("] \n");
    //	printf("N = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", noise[i]);
    //	}
    //	printf("] \n");
    //
    //	puts("done.");
    //
    //	printf("S = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", spd[i]);
    //	}
    //	printf("] \n");
    //	assert(lingot_signal_quick_select(spd, 1) == 1.0);
    //	assert(lingot_signal_quick_select(spd, 2) == 1.0);
    //	assert(lingot_signal_quick_select(spd, 3) == 2.0);
    //	assert(lingot_signal_quick_select(spd, 4) == 2.0);
    //	assert(lingot_signal_quick_select(spd, 5) == 3.0);
    //	assert(lingot_signal_quick_select(spd, 6) == 3.0);
    //	printf("S = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", spd[i]);
    //	}
    //	printf("] \n");

    free(spd);
    free(noise);

    N = 512;
    i = 0;
    n = 30;
    spd = malloc((size_t) N * sizeof(LINGOT_FLT));
    noise = malloc((size_t) N * sizeof(LINGOT_FLT));

    for (i = 0; i < N; i++) {
        spd[i] = N - i;
        noise[i] = -1.0;
    }

    //	printf("S = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", spd[i]);
    //	}
    //	printf("] \n");

    //	double m;
    //
    //	tic();
    //	m = lingot_signal_quick_select(spd, 512);
    //	toc();
    //
    //	printf("m = %f\n", m);

    //	printf("S = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", spd[i]);
    //	}
    //	printf("] \n");

    //	tic();
    //	m = lingot_signal_quick_select(spd, 512);
    //	toc();

    //	printf("m = %f\n", m);

    //	printf("S = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", spd[i]);
    //	}
    //	printf("] \n");

    // -----------------

    for (i = 0; i < N; i++) {
        spd[i] = N - i;
        noise[i] = -1.0;
    }

    tic();
    lingot_signal_compute_noise_level(spd, N, n, noise);
    toc();

    //	printf("N = [");
    //	for (i = 0; i < N; i++) {
    //		printf(" %f ", noise[i]);
    //	}
    //	printf("] \n");

    tic();
    for (i = 0; i < 10000; i++) {
        lingot_signal_compute_noise_level(spd, N, n, noise);
    }
    toc();

    free(spd);
    free(noise);
}

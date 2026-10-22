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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "lingot-test.h"
#include "lingot-filter.h"

void lingot_test_filter(void) {

    lingot_filter_t filter;
    lingot_filter_cheby_design(&filter, 8, 0.5, 0.9 / 21);

    CU_ASSERT_EQUAL(filter.N, 8);
    double tol = 1e-10;
    CU_ASSERT_DOUBLE_EQUAL(filter.a[0], 1.000000000000000e+00, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[1], -7.809937440914638e+00, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[2], 2.672218495005448e+01, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[3], -5.231813884768846e+01, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[4], 6.410652263625612e+01, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[5], -5.034053356082422e+01, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[6], 2.473976892695576e+01, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[7], -6.956880969005340e+00, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[8], 8.570143075418742e-01, tol);

    CU_ASSERT_DOUBLE_EQUAL(filter.b[0], 8.760492557384442e-12, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[1], 7.008394045907554e-11, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[2], 2.452937916067644e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[3], 4.905875832135288e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[4], 6.132344790169110e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[5], 4.905875832135288e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[6], 2.452937916067644e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[7], 7.008394045907554e-11, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[8], 8.760492557384442e-12, tol);

}

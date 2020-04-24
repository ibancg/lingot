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

void lingot_test_filter(void) {

    lingot_filter_t filter;
    lingot_filter_cheby_design(&filter, 8, 0.5, 0.9 / 21);

    CU_ASSERT_EQUAL(filter.N, 8);
    double tol = 1e-10;
    CU_ASSERT_DOUBLE_EQUAL(filter.a[0], 1.0, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[1], -7.809937443633682, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[2], 26.72218497045733, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[3], -52.3181389128507, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[4], 64.10652275124353, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[5], -50.34053368203738, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[6], 24.7397690033508, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[7], -6.95688099567813, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.a[8], 0.8570143115238185, tol);

    CU_ASSERT_DOUBLE_EQUAL(filter.b[0], 8.760493236323022e-12, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[1], 7.008394589058418e-11, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[2], 2.452938106170446e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[3], 4.905876212340893e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[4], 6.132345265426116e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[5], 4.905876212340893e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[6], 2.452938106170446e-10, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[7], 7.008394589058418e-11, tol);
    CU_ASSERT_DOUBLE_EQUAL(filter.b[8], 8.760493236323022e-12, tol);

}

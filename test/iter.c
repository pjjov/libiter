/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/iter.h>
#include <pf_assert.h>
#include <pf_test.h>

int test_iter_from_array(int seed, int rep) {
    int out, a[] = { 1, 2, 3, 4, 5 };
    iter_t storage;

    iter(int) it = iter_from_array(&storage, a, 5);
    pf_assert_not_null(it);

    for (size_t i = 0; i < 5; i++) {
        pf_assert_ok(iter_next(it, &out));
        pf_assert(out == a[i]);
    }

    pf_assert(ITER_ENODATA == iter_next(it, &out));

    return 0;
}

int test_iter_to_array(int seed, int rep) {
    int a[5] = { 1, 2, 3, 4, 5 };
    int out, b[5] = { 0 };
    iter_t storage;

    iter(int) it = iter_from_array(&storage, a, 5);
    pf_assert_not_null(it);

    pf_assert(5 == iter_to_array(it, &b[0], 5));
    pf_assert_memcmp(a, b, sizeof(int) * 5);
    pf_assert(ITER_ENODATA == iter_next(it, &out));

    return 0;
}

pf_test suite_iter[] = {
    { test_iter_from_array, "/iter/from_array", 1 },
    { test_iter_to_array, "/iter/to_array", 1 },
    { 0 },
};

/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/vector.h>
#include <pf_assert.h>
#include <pf_test.h>
#include <stdint.h>

int test_vector_init(int seed, int repetition) {
    vector_t v;

    vector(int) out = vector_init(int, &v, NULL);
    pf_assert_not_null(out);
    pf_assert_not_null(vector_allocator(out));
    pf_assert_null(vector_items(out));
    pf_assert(0 == vector_length(out));
    pf_assert(0 == vector_capacity(out));

    vector_free(&v);
    return 0;
}

int test_vector_create(int seed, int repetition) {
    vector(int) v;

    v = vector_create(int, NULL);
    pf_assert_not_null(v);
    pf_assert_null(vector_items(v));
    pf_assert(0 == vector_length(v));
    pf_assert(0 == vector_capacity(v));

    vector_destroy(v);
    return 0;
}

pf_test suite_vector[] = {
    { test_vector_init, "/vector/init", 1 },
    { test_vector_create, "/vector/create", 1 },
    { 0 },
};

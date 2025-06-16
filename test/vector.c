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

int test_vector_wrap(int seed, int repetition) {
    int a[] = { 0, 1, 2 };
    vector(int) v = vector_wrap(a, 3, NULL);
    pf_assert_not_null(v);

    pf_assert(vector_items(v) == a);
    pf_assert(vector_get(v, 0) == &a[0]);
    pf_assert(vector_get(v, 1) == &a[1]);
    pf_assert(vector_get(v, 2) == &a[2]);

    vector_destroy(v);
    return 0;
}

int test_vector_reserve(int seed, int repetition) {
    vector(int) v = vector_create(int, NULL);
    pf_assert_not_null(v);
    pf_assert(vector_capacity(v) == 0);

    pf_assert_ok(vector_reserve(v, 2));
    pf_assert(vector_capacity(v) >= 2);

    vector_destroy(v);
    return 0;
}

int test_vector_resize(int seed, int repetition) {
    vector(int) v = vector_create(int, NULL);
    pf_assert_not_null(v);
    pf_assert(vector_capacity(v) == 0);
    pf_assert_null(vector_items(v));

    pf_assert_ok(vector_resize(v, 10));
    pf_assert(vector_capacity(v) == 10);

    pf_assert_ok(vector_reserve(v, 5));
    pf_assert(vector_capacity(v) == 10);

    pf_assert_ok(vector_shrink(v));
    pf_assert(vector_capacity(v) == 0);
    pf_assert_null(vector_items(v));

    vector_destroy(v);
    return 0;
}

int test_vector_insert(int seed, int repetition) {
    int a[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    vector(int) v = vector_create(int, NULL);
    pf_assert_not_null(v);

    pf_assert_ok(vector_insert(v, &a[0], 0, 3));
    pf_assert(vector_length(v) == 3);

    pf_assert_ok(vector_insert(v, &a[8], 3, 2));
    pf_assert(vector_length(v) == 5);

    pf_assert_ok(vector_insert(v, &a[3], 3, 5));
    pf_assert(vector_length(v) == 10);
    pf_assert_memcmp(a, vector_items(v), sizeof(a));

    vector_destroy(v);
    return 0;
}

int test_vector_try_insert(int seed, int repetition) {
    int a[] = { 1, 2, 3, 4, 5 };
    vector(int) v = vector_create(int, NULL);
    pf_assert_not_null(v);

    pf_assert_ok(vector_resize(v, 4));
    pf_assert_ok(vector_try_insert(v, (int *)a, 0, 2));
    pf_assert(vector_length(v) == 2);
    pf_assert_memcmp(a, vector_items(v), 2 * sizeof(int));

    pf_assert(ITER_ENOMEM == vector_try_insert(v, (int *)a, 0, 3));
    pf_assert(ITER_EINVAL == vector_try_insert(v, (int *)a, 3, 2));
    pf_assert(ITER_EINVAL == vector_try_insert(v, (int *)NULL, 0, 1));

    vector_destroy(v);
    return 0;
}

pf_test suite_vector[] = {
    { test_vector_init, "/vector/init", 1 },
    { test_vector_create, "/vector/create", 1 },
    { test_vector_wrap, "/vector/wrap", 1 },
    { test_vector_resize, "/vector/resize", 1 },
    { test_vector_reserve, "/vector/reserve", 1 },
    { test_vector_insert, "/vector/insert", 1 },
    { test_vector_try_insert, "/vector/try_insert", 1 },
    { 0 },
};

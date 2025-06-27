/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/vector.h>
#include <pf_assert.h>
#include <pf_test.h>
#include <stdint.h>

int test_vector_init(int seed, int rep) {
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

int test_vector_create(int seed, int rep) {
    vector(int) v;

    v = vector_create(int, NULL);
    pf_assert_not_null(v);
    pf_assert_null(vector_items(v));
    pf_assert(0 == vector_length(v));
    pf_assert(0 == vector_capacity(v));

    vector_destroy(v);
    return 0;
}

int test_vector_wrap(int seed, int rep) {
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

int test_vector_reserve(int seed, int rep) {
    vector(int) v = vector_create(int, NULL);
    pf_assert_not_null(v);
    pf_assert(vector_capacity(v) == 0);

    pf_assert_ok(vector_reserve(v, 2));
    pf_assert(vector_capacity(v) >= 2);

    vector_destroy(v);
    return 0;
}

int test_vector_resize(int seed, int rep) {
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

int test_vector_insert(int seed, int rep) {
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

int test_vector_try_insert(int seed, int rep) {
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

int test_vector_remove(int seed, int rep) {
    int a[] = { 1, 2, 3, 4, 5 };
    vector(int) v = vector_wrap(a, 5, NULL);
    pf_assert_not_null(v);

    pf_assert_ok(vector_remove(v, 0, 2));
    pf_assert(vector_length(v) == 3);
    pf_assert(a[0] == *vector_get(v, 0));
    pf_assert(a[1] == *vector_get(v, 1));
    pf_assert(a[2] == *vector_get(v, 2));

    pf_assert_ok(vector_pop(v, 2));
    pf_assert(vector_length(v) == 1);
    pf_assert(3 == *vector_get(v, 0));

    pf_assert(ITER_EINVAL == vector_pop(v, 2));
    pf_assert(ITER_EINVAL == vector_remove(v, 2, 1));
    pf_assert(ITER_EINVAL == vector_remove(v, 0, 30));

    vector_destroy(v);
    return 0;
}

int test_vector_from_array(int seed, int rep) {
    int a[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    vector(int) v1 = vector_from_array(a, 10, NULL);
    pf_assert_not_null(v1);
    pf_assert(vector_length(v1) == 10);
    pf_assert(vector_bytes_used(v1) == sizeof(a));

    for (size_t i = 0; i < 10; i++)
        pf_assert(*vector_get(v1, i) == a[i]);

    int b[] = { 0 };
    vector(int) v2 = vector_from_array(b, 0, NULL);
    pf_assert_null(v2);

    vector_destroy(v1);
    vector_destroy(v2);
    return 0;
}

int test_vector_clone(int seed, int rep) {
    int a[] = { 1, 2, 3, 4, 5 };
    vector(int) vector = vector_from_array(a, 5, NULL);
    pf_assert_not_null(vector);

    vector(int) cloned = vector_clone(vector, NULL);
    pf_assert_not_null(cloned);
    pf_assert(vector_length(vector) == vector_length(cloned));
    pf_assert(vector_items(vector) != vector_items(cloned));

    vector_destroy(vector);
    vector_destroy(cloned);
    return 0;
}

int test_vector_index(int seed, int repetition) {
    int a[] = { 0, 1, 2 };
    vector(int) v = vector_from_array(a, 3, NULL);
    pf_assert_not_null(v);

    pf_assert(0 == vector_index(v, vector_get(v, 0)));
    pf_assert(1 == vector_index(v, vector_get(v, 1)));
    pf_assert(2 == vector_index(v, vector_get(v, 2)));

    pf_assert(vector_length(v) == vector_index(v, (int *)NULL));

    vector_destroy(v);
    return 0;
}

int test_vector_insert_at(int seed, int repetition) {
    int a[] = { 1, 2, 3, 4, 5 };
    vector(int) v = vector_with_capacity(int, 5, NULL);
    pf_assert_not_null(v);

    pf_assert_ok(vector_insert_at(v, (int *)a, vector_items(v), 3));
    pf_assert(vector_length(v) == 3);
    pf_assert_memcmp(a, vector_items(v), 3 * sizeof(int));

    pf_assert_ok(
        vector_insert_at(v, &a[3], vector_slot(v, vector_length(v)), 2)
    );

    pf_assert(vector_length(v) == 5);
    pf_assert_memcmp(a, vector_items(v), 5 * sizeof(int));

    pf_assert(ITER_EINVAL == vector_insert_at(v, &a[0], (int *)NULL, 3));
    pf_assert(
        ITER_EINVAL == vector_insert_at(v, (int *)a, vector_get(v, 0), 0)
    );

    vector_destroy(v);
    return 0;
}

int test_vector_remove_at(int seed, int repetition) {
    int a[] = { 1, 2, 3, 4, 5 };
    vector(int) v = vector_from_array(a, 5, NULL);
    pf_assert_not_null(v);

    pf_assert_ok(vector_remove_at(v, vector_get(v, 0), 2));
    pf_assert(vector_length(v) == 3);
    pf_assert_memcmp(&a[2], vector_items(v), 3 * sizeof(int));

    pf_assert_ok(vector_remove_at(v, vector_get(v, vector_length(v) - 1), 1));
    pf_assert(vector_length(v) == 2);
    pf_assert_memcmp(&a[2], vector_items(v), 2 * sizeof(int));

    pf_assert(ITER_EINVAL == vector_remove_at(v, &a[0], 2));
    pf_assert(ITER_EINVAL == vector_remove_at(v, (int *)NULL, 2));
    pf_assert(ITER_EINVAL == vector_remove_at(v, &a[0], 0));
    pf_assert(ITER_EINVAL == vector_remove_at((vector(int))NULL, &a[0], 2));

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
    { test_vector_remove, "/vector/remove", 1 },
    { test_vector_from_array, "/vector/from_array", 1 },
    { test_vector_clone, "/vector/clone", 1 },
    { test_vector_index, "/vector/index", 1 },
    { test_vector_insert_at, "/vector/insert_at", 1 },
    { test_vector_remove_at, "/vector/remove_at", 1 },
    { 0 },
};

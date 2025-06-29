/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/error.h>
#include <iter/hashmap.h>
#include <pf_assert.h>
#include <pf_test.h>
#include <stdint.h>

int test_hashmap_init(int seed, int rep) {
    hashmap_t storage;

    hashmap(int, double) map = hashmap_init(int, double, &storage, NULL);
    pf_assert_not_null(map);
    pf_assert(hashmap_count(map) == 0);
    pf_assert(hashmap_capacity(map) == 0);

    hashmap_free(&storage);
    return 0;
}

int test_hashmap_create(int seed, int rep) {
    hashmap(int, double) map = hashmap_create(int, double, NULL);

    pf_assert_not_null(map);
    pf_assert(hashmap_count(map) == 0);
    pf_assert(hashmap_capacity(map) == 0);

    hashmap_destroy(map);
    return 0;
}

int test_hashmap_reserve(int seed, int rep) {
    hashmap(int, double) map = hashmap_create(int, double, NULL);
    pf_assert_not_null(map);

    pf_assert_ok(hashmap_reserve(map, 10));
    pf_assert(hashmap_capacity(map) >= 10);
    pf_assert(hashmap_count(map) == 0);

    hashmap_destroy(map);
    return 0;
}

int test_hashmap_get_set(int seed, int rep) {
    int keys[5] = { 1, 2, 3, 4, 5 };
    double values[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };

    hashmap(int, double) map = hashmap_create(int, double, NULL);
    pf_assert_not_null(map);

    for (size_t i = 0; i < 5; i++) {
        pf_assert_ok(hashmap_set(map, &keys[i], &values[i]));
    }

    for (size_t i = 0; i < 5; i++) {
        pf_assert(*hashmap_get(map, &keys[i]) == values[i]);
    }

    pf_assert_null(hashmap_get(map, (int *)NULL));
    pf_assert(ITER_EINVAL == hashmap_set(map, (int *)NULL, (double *)NULL));
    hashmap_destroy(map);
    return 0;
}

int test_hashmap_insert_remove(int seed, int rep) {
    int keys[5] = { 1, 2, 3, 4, 5 };
    double values[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };

    hashmap(int, double) map = hashmap_create(int, double, NULL);
    pf_assert_not_null(map);

    for (size_t i = 0; i < 5; i++)
        pf_assert_ok(hashmap_insert(map, &keys[i], &values[i]));

    for (size_t i = 0; i < 5; i++) {
        pf_assert(ITER_EEXIST == hashmap_insert(map, &keys[i], &values[i]));
        pf_assert(*hashmap_get(map, &keys[i]) == values[i]);
    }

    for (size_t i = 0; i < 5; i++)
        pf_assert_ok(hashmap_remove(map, &keys[i]));

    for (size_t i = 0; i < 5; i++) {
        pf_assert_null(hashmap_get(map, &keys[i]));
        pf_assert(ITER_ENOENT == hashmap_remove(map, &keys[i]));
    }

    pf_assert(ITER_EINVAL == hashmap_insert(map, (int *)NULL, (double *)NULL));
    pf_assert(ITER_EINVAL == hashmap_remove(map, (int *)NULL));

    hashmap_destroy(map);
    return 0;
}

pf_test suite_hashmap[] = {
    { test_hashmap_init, "/hashmap/init", 1 },
    { test_hashmap_create, "/hashmap/create", 1 },
    { test_hashmap_reserve, "/hashmap/reserve", 1 },
    { test_hashmap_get_set, "/hashmap/get_set", 1 },
    { test_hashmap_insert_remove, "/hashmap/insert_remove", 1 },
    { 0 },
};

/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

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

pf_test suite_hashmap[] = {
    { test_hashmap_init, "/hashmap/init", 1 },
    { test_hashmap_create, "/hashmap/create", 1 },
    { 0 },
};

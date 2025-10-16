/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/error.h>
#include <iter/hashmap.h>
#include <iter/iter.h>
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

int test_hashmap_from_arrays(int seed, int rep) {
    int keys[5] = { 1, 2, 3, 4, 5 };
    double values[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };

    hashmap(int, double) map = hashmap_from_arrays(keys, values, 5, NULL);
    pf_assert_not_null(map);

    for (int i = 0; i < 5; i++) {
        pf_assert_not_null(hashmap_get(map, &keys[i]));
        pf_assert(values[i] == *hashmap_get(map, &keys[i]));
    }

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

static int each_pair(void *key, void *value, void *user) {
    *(double *)value -= *(int *)key;
    return 0;
}

int test_hashmap_each(int seed, int rep) {
    int keys[5] = { 1, 2, 3, 4, 5 };
    double values[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };

    hashmap(int, double) map = hashmap_create(int, double, NULL);
    pf_assert_not_null(map);

    for (size_t i = 0; i < 5; i++)
        pf_assert_ok(hashmap_insert(map, &keys[i], &values[i]));

    pf_assert_ok(hashmap_each(map, each_pair, NULL));
    for (size_t i = 0; i < 5; i++) {
        pf_assert_not_null(hashmap_get(map, &keys[i]));
        pf_assert(values[i] - keys[i] == *hashmap_get(map, &keys[i]));
    }

    hashmap_destroy(map);
    return 0;
}

int filter_pair(void *key, void *item, void *user) {
    return *(double *)item < *(double *)user;
}

int test_hashmap_filter(int seed, int rep) {
    int keys[5] = { 1, 2, 3, 4, 5 };
    double values[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };

    hashmap(int, double) map = hashmap_create(int, double, NULL);
    pf_assert_not_null(map);

    for (size_t i = 0; i < 5; i++)
        pf_assert_ok(hashmap_insert(map, &keys[i], &values[i]));

    double threshold = 3;
    pf_assert_ok(hashmap_filter(map, filter_pair, &threshold));
    pf_assert_not_null(hashmap_get(map, &keys[0]));
    pf_assert_not_null(hashmap_get(map, &keys[1]));
    pf_assert(values[0] == *hashmap_get(map, &keys[0]));
    pf_assert(values[1] == *hashmap_get(map, &keys[1]));
    pf_assert_null(hashmap_get(map, &keys[2]));
    pf_assert_null(hashmap_get(map, &keys[3]));
    pf_assert_null(hashmap_get(map, &keys[4]));

    hashmap_destroy(map);
    return 0;
}

int test_hashmap_iter(int seed, int rep) {
    int keys[5] = { 1, 2, 3, 4, 5 };
    double values[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    iter_t storage;

    hashmap(int, double) map = hashmap_create(int, double, NULL);
    pf_assert_not_null(map);

    for (size_t i = 0; i < 5; i++)
        pf_assert_ok(hashmap_insert(map, &keys[i], &values[i]));

    iter(double) it = hashmap_iter(map, &storage);
    pf_assert_not_null(it);

    double out, sum = 0;
    size_t count = 0;
    for (; 0 == iter_next(it, &out); count++) {
        pf_assert(count < 5);
        sum += out;
    }

    pf_assert(count == 5);
    pf_assert(sum == 16.5);
    pf_assert(ITER_ENODATA == iter_next(it, &out));

    hashmap_destroy(map);
    return 0;
}

pf_test suite_hashmap[] = {
    { test_hashmap_init, "/hashmap/init", 1 },
    { test_hashmap_create, "/hashmap/create", 1 },
    { test_hashmap_from_arrays, "/hashmap/from_arrays", 1 },
    { test_hashmap_reserve, "/hashmap/reserve", 1 },
    { test_hashmap_get_set, "/hashmap/get_set", 1 },
    { test_hashmap_insert_remove, "/hashmap/insert_remove", 1 },
    { test_hashmap_each, "/hashmap/each", 1 },
    { test_hashmap_filter, "/hashmap/filter", 1 },
    { test_hashmap_iter, "/hashmap/iter", 1 },
    { 0 },
};

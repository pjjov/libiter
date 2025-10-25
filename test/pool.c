/*
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0

    Copyright 2025 Predrag Jovanović

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <pf_assert.h>
#include <pf_test.h>

#include <iter/iter.h>
#include <iter/pool.h>

int test_pool_init(int seed, int rep) {
    pool_t storage;

    pool(int) p = pool_init(int, &storage, NULL);
    pf_assert_not_null(p);
    pf_assert(pool_count(p) == 0);
    pf_assert(pool_capacity(p) == 0);

    pf_assert_ok(pool_reserve(p, 10));
    pf_assert(pool_count(p) == 0);
    pf_assert(pool_capacity(p) >= 10);

    pool_free(&storage);
    return 0;
}

int test_pool_create(int seed, int rep) {
    pool(int) p = pool_create(int, NULL);
    pf_assert_not_null(p);
    pf_assert(pool_count(p) == 0);
    pf_assert(pool_capacity(p) == 0);

    pf_assert_ok(pool_reserve(p, 10));
    pf_assert(pool_count(p) == 0);
    pf_assert(pool_capacity(p) >= 10);

    pool_destroy(p);
    return 0;
}

int test_pool_give_take(int seed, int rep) {
    pool(int) p = pool_create(int, NULL);
    pf_assert_not_null(p);

    int *item = pool_take(p);
    pf_assert_not_null(item);
    pf_assert_ok(pool_give(p, item));
    pf_assert(ITER_ENOENT == pool_give(p, item));
    pf_assert(ITER_EINVAL == pool_give(p, (int *)NULL));

    pool_destroy(p);
    return 0;
}

int test_pool_index(int seed, int rep) {
    int *items[10] = { 0 };
    pool(int) p = pool_create(int, NULL);
    pf_assert_not_null(p);

    for (int i = 0; i < 10; i++) {
        items[i] = pool_take(p);
        pf_assert_not_null(items[i]);
    }

    for (int i = 0; i < 10; i++) {
        size_t index = pool_to_index(p, items[i]);
        pf_assert_not_null(pool_from_index(p, index));
    }

    for (int i = 0; i < 10; i++) {
        pf_assert_ok(pool_give(p, items[i]));
        pf_assert(ITER_ENOENT == pool_give(p, items[i]));
    }

    pool_destroy(p);
    return 0;
}

int test_pool_iter(int seed, int repetition) {
    pool(int) p = pool_create(int, NULL);
    pf_assert_not_null(p);

    for (int i = 0; i < 20; i++) {
        int *item = pool_take(p);
        pf_assert_not_null(item);
        *item = i;
        if (i % 2 == 0)
            pf_assert_ok(pool_give(p, item));
    }
    pf_assert(10 == pool_count(p));

    iter_t storage;
    iter(int) it = pool_iter(p, &storage);
    pf_assert_not_null(it);

    int result;
    for (int i = 1; i < 20; i += 2) {
        pf_assert_ok(iter_next(it, &result));
        pf_assert(result % 2 == 1);
    }

    pool_destroy(p);
    return 0;
}

int test_pool_resize(int seed, int rep) {
    pool(void *) p = pool_create(void *, NULL);
    pf_assert_not_null(p);

    for (int i = 0; i < 2048; i++) {
        void **item = pool_take(p);
        pf_assert_not_null(item);
    }

    pf_assert(2048 == pool_count(p));
    pool_destroy(p);
    return 0;
}

pf_test suite_pool[] = {
    { test_pool_init, "/pool/init", 1 },
    { test_pool_create, "/pool/create", 1 },
    { test_pool_give_take, "/pool/give_take", 1 },
    { test_pool_index, "/pool/index", 1 },
    { test_pool_iter, "/pool/iter", 1 },
    { test_pool_resize, "/pool/resize", 1 },
    { 0 },
};

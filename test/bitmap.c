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

#include <iter/bitmap.h>
#include <iter/error.h>

int test_bitmap_init(int seed, int rep) {
    bitmap_t storage;

    bitmap_t *b = bitmap_init(&storage, NULL);
    pf_assert_not_null(b);
    pf_assert(bitmap_length(b) == 0);
    pf_assert(bitmap_capacity(b) == 0);

    pf_assert_ok(bitmap_reserve(b, 10));
    pf_assert(bitmap_length(b) == 0);
    pf_assert(bitmap_capacity(b) >= 10);

    bitmap_free(&storage);
    return 0;
}

int test_bitmap_create(int seed, int rep) {
    bitmap_t *b = bitmap_create(NULL);
    pf_assert_not_null(b);
    pf_assert(bitmap_length(b) == 0);
    pf_assert(bitmap_capacity(b) == 0);

    pf_assert_ok(bitmap_reserve(b, 10));
    pf_assert(bitmap_length(b) == 0);
    pf_assert(bitmap_capacity(b) >= 10);

    bitmap_destroy(b);
    return 0;
}

int test_bitmap_get_set(int seed, int rep) {
    bitmap_t storage;

    bitmap_t *b = bitmap_init(&storage, NULL);
    pf_assert_not_null(b);
    pf_assert_ok(bitmap_resize(b, 16));

    for (size_t i = 0; i < 16; i++)
        pf_assert(bitmap_get(b, i) == 0);

    pf_assert(bitmap_set(b, 3, 1) == 0);
    pf_assert(bitmap_get(b, 3) == 1);

    pf_assert(bitmap_set(b, 3, 0) == 0);
    pf_assert(bitmap_get(b, 3) == 0);

    pf_assert(bitmap_toggle(b, 5) == 0);
    pf_assert(bitmap_get(b, 5) == 1);

    pf_assert(bitmap_toggle(b, 5) == 1);
    pf_assert(bitmap_get(b, 5) == 0);

    pf_assert(bitmap_get(b, 16) == ITER_EINVAL);
    pf_assert(bitmap_set(b, 99, 1) == ITER_EINVAL);

    bitmap_free(b);
    return 0;
}

int test_bitmap_getn_setn(int seed, int rep) {
    bitmap_t storage;

    bitmap_t *b = bitmap_init(&storage, NULL);
    pf_assert_not_null(b);
    pf_assert_ok(bitmap_resize(b, 16));

    pf_assert_ok(bitmap_setn(b, 1, 4, 8));
    for (size_t i = 4; i < 12; i++)
        pf_assert(bitmap_get(b, i) == 1);
    for (size_t i = 0; i < 4; i++)
        pf_assert(bitmap_get(b, i) == 0);
    for (size_t i = 12; i < 16; i++)
        pf_assert(bitmap_get(b, i) == 0);

    char buf[2] = { 0 };
    pf_assert_ok(bitmap_getn(b, buf, 4, 8));
    pf_assert((unsigned char)buf[0] == 0xFF);

    pf_assert_ok(bitmap_clear(b));

    pf_assert_ok(bitmap_resize(b, 8));
    pf_assert_ok(bitmap_togglen(b, 0, 8));
    for (size_t i = 0; i < 8; i++)
        assert(bitmap_get(b, i) == 1);

    pf_assert_ok(bitmap_togglen(b, 0, 8));
    for (size_t i = 0; i < 8; i++)
        pf_assert(bitmap_get(b, i) == 0);

    bitmap_free(b);
    return 0;
}

int test_bitmap_binary(int seed, int rep) {
    bitmap_t storageA, storageB;
    bitmap_t *a = bitmap_init(&storageA, NULL);
    bitmap_t *b = bitmap_init(&storageB, NULL);
    pf_assert_ok(bitmap_resize(a, 8));
    pf_assert_ok(bitmap_resize(b, 8));

    pf_assert_ok(bitmap_set(b, 2, 1));
    pf_assert_ok(bitmap_inv(b));
    pf_assert(bitmap_get(b, 2) == 0);

    for (size_t i = 0; i < 8; i++) {
        if (i != 2)
            pf_assert(bitmap_get(b, i) == 1);
    }

    pf_assert(bitmap_inv(NULL) == ITER_EINVAL);

    pf_assert_ok(bitmap_clear(b));

    pf_assert_ok(bitmap_set(a, 0, 1));
    pf_assert_ok(bitmap_set(b, 1, 1));
    pf_assert_ok(bitmap_or(a, b));
    pf_assert(bitmap_get(a, 0) == 1);
    pf_assert(bitmap_get(a, 1) == 1);

    pf_assert_ok(bitmap_clear(b));

    pf_assert_ok(bitmap_set(a, 0, 1));
    pf_assert_ok(bitmap_set(a, 1, 1));
    pf_assert_ok(bitmap_set(b, 1, 1));
    pf_assert_ok(bitmap_and(a, b));
    pf_assert(bitmap_get(a, 0) == 0);
    pf_assert(bitmap_get(a, 1) == 1);

    pf_assert_ok(bitmap_clear(b));

    pf_assert_ok(bitmap_set(a, 0, 1));
    pf_assert_ok(bitmap_set(a, 1, 1));
    pf_assert_ok(bitmap_set(b, 1, 1));
    pf_assert_ok(bitmap_set(b, 2, 1));
    pf_assert_ok(bitmap_xor(a, b));
    pf_assert(bitmap_get(a, 0) == 1);
    pf_assert(bitmap_get(a, 1) == 0);
    pf_assert(bitmap_get(a, 2) == 1);

    bitmap_free(a);
    bitmap_free(b);
    return 0;
}

int test_bitmap_counting(int seed, int rep) {
    bitmap_t storage;
    bitmap_t *b = bitmap_init(&storage, NULL);
    pf_assert_ok(bitmap_resize(b, 8));

    pf_assert(bitmap_ctz(b) == 8);
    pf_assert_ok(bitmap_set(b, 3, 1));
    pf_assert(bitmap_ctz(b) == 3);

    pf_assert_ok(bitmap_clear(b));

    pf_assert(bitmap_clz(b) == 8);
    pf_assert_ok(bitmap_set(b, 5, 1));
    pf_assert(bitmap_clz(b) == 2);

    bitmap_free(b);
    return 0;
}

int test_bitmap_shift(int seed, int rep) {
    bitmap_t storage;
    bitmap_t *b = bitmap_init(&storage, NULL);
    pf_assert_ok(bitmap_resize(b, 8));

    pf_assert_ok(bitmap_set(b, 0, 1));
    pf_assert_ok(bitmap_shl(b, 1));
    pf_assert(bitmap_get(b, 0) == 0);
    pf_assert(bitmap_get(b, 1) == 1);
    pf_assert(bitmap_shr(NULL, 1) == ITER_EINVAL);

    pf_assert_ok(bitmap_clear(b));

    pf_assert_ok(bitmap_set(b, 7, 1));
    pf_assert_ok(bitmap_shr(b, 1));
    pf_assert(bitmap_get(b, 7) == 0);
    pf_assert(bitmap_get(b, 6) == 1);
    pf_assert(bitmap_shl(NULL, 1) == ITER_EINVAL);

    bitmap_free(b);
    return 0;
}

int test_bitmap_rotate(int seed, int rep) {
    bitmap_t storage;
    bitmap_t *b = bitmap_init(&storage, NULL);
    pf_assert_ok(bitmap_resize(b, 8));

    pf_assert_ok(bitmap_set(b, 0, 1));
    pf_assert_ok(bitmap_rotr(b, 1));
    pf_assert(bitmap_get(b, 0) == 0);
    pf_assert(bitmap_get(b, 7) == 1);
    pf_assert(bitmap_rotr(NULL, 1) == ITER_EINVAL);

    pf_assert_ok(bitmap_clear(b));

    pf_assert_ok(bitmap_set(b, 7, 1));
    pf_assert_ok(bitmap_rotl(b, 1));
    pf_assert(bitmap_get(b, 7) == 0);
    pf_assert(bitmap_get(b, 0) == 1);
    pf_assert(bitmap_rotl(NULL, 1) == ITER_EINVAL);

    bitmap_free(b);
    return 0;
}

pf_test suite_bitmap[] = {
    { test_bitmap_init, "/bitmap/init", 1 },
    { test_bitmap_create, "/bitmap/create", 1 },
    { test_bitmap_get_set, "/bitmap/get_set", 1 },
    { test_bitmap_getn_setn, "/bitmap/getn_setn", 1 },
    { test_bitmap_binary, "/bitmap/binary", 1 },
    { test_bitmap_counting, "/bitmap/counting", 1 },
    { test_bitmap_shift, "/bitmap/shift", 1 },
    { test_bitmap_rotate, "/bitmap/rotate", 1 },
    { 0 },
};

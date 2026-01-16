/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef ITER_BITMAP_H
#define ITER_BITMAP_H

#include <stddef.h>

typedef struct allocator_t allocator_t;

enum {
    BITMAP_FALSE = 0,
    BITMAP_TRUE = 1,
    BITMAP_CLEAR = 0,
    BITMAP_SET = 1,
};

typedef struct bitmap_t {
    size_t length;
    size_t capacity;
    void *buffer;
    allocator_t *allocator;
} bitmap_t;

bitmap_t *bitmap_init(bitmap_t *out, allocator_t *allocator);
bitmap_t *bitmap_create(allocator_t *allocator);
void bitmap_destroy(bitmap_t *map);
void bitmap_free(bitmap_t *map);

int bitmap_reserve(bitmap_t *map, size_t count);
int bitmap_resize(bitmap_t *map, size_t capacity);

#endif

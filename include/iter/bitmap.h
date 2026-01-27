/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef ITER_BITMAP_H
#define ITER_BITMAP_H

#ifndef ITER_API
    #define ITER_API static inline
#endif

#include <stddef.h>

typedef struct allocator_t allocator_t;

enum {
    BITMAP_CLEAR = 0,
    BITMAP_SET = 1,
};

typedef struct bitmap_t {
    size_t length;
    void *buffer;
    allocator_t *allocator;

    union {
        size_t capacity;
        size_t offset;
    } as;
} bitmap_t;

bitmap_t *bitmap_init(bitmap_t *out, allocator_t *allocator);
bitmap_t *bitmap_create(allocator_t *allocator);
int bitmap_slice(bitmap_t *dst, const bitmap_t *src, size_t from, size_t to);
void bitmap_destroy(bitmap_t *map);
void bitmap_free(bitmap_t *map);

ITER_API size_t bitmap_length(bitmap_t *map) { return map ? map->length : 0; }

ITER_API size_t bitmap_capacity(bitmap_t *map) {
    if (!map)
        return 0;

    return map->allocator ? map->as.capacity : map->length;
}

ITER_API size_t bitmap_offset(bitmap_t *map) {
    if (!map || map->allocator)
        return 0;
    return map->as.offset;
}

ITER_API allocator_t *bitmap_allocator(bitmap_t *map) {
    return map ? map->allocator : NULL;
}

int bitmap_reserve(bitmap_t *map, size_t count);
int bitmap_resize(bitmap_t *map, size_t length);

int bitmap_get(bitmap_t *map, size_t i);
int bitmap_set(bitmap_t *map, size_t i, int value);
int bitmap_toggle(bitmap_t *map, size_t i);

int bitmap_inv(bitmap_t *map);
int bitmap_or(bitmap_t *dst, bitmap_t *src);
int bitmap_and(bitmap_t *dst, bitmap_t *src);
int bitmap_xor(bitmap_t *dst, bitmap_t *src);
int bitmap_shr(bitmap_t *map, size_t count);
int bitmap_shl(bitmap_t *map, size_t count);
int bitmap_rotr(bitmap_t *map, int count);
int bitmap_rotl(bitmap_t *map, int count);

size_t bitmap_ctz(bitmap_t *map);
size_t bitmap_clz(bitmap_t *map);
size_t bitmap_cto(bitmap_t *map);
size_t bitmap_clo(bitmap_t *map);
size_t bitmap_ftz(bitmap_t *map);
size_t bitmap_flz(bitmap_t *map);
size_t bitmap_fto(bitmap_t *map);
size_t bitmap_flo(bitmap_t *map);
size_t bitmap_popcount(bitmap_t *map);
size_t bitmap_zerocount(bitmap_t *map);
int bitmap_parity(bitmap_t *map);

#endif

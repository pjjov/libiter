/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/bitmap.h>

#include <allocator.h>
#include <iter/error.h>
#include <pf_macro.h>

extern allocator_t *libiter_allocator;

#ifndef BITMAP_INT
    #include <limits.h>
    #include <stdint.h>
    #define BITMAP_INT uint64_t
    #define BITMAP_INT_BITS 64
    #define BITMAP_INT_MAX UINT64_MAX
    #define BITMAP_INT_MIN UINT64_MIN
#endif

typedef BITMAP_INT bitmap_int_t;

#define BITMAP_GROWTH(old, count) (2 * ((old) + (count)))
#define BIT(value, offset) (((bitmap_int_t)value) << (offset))

bitmap_t *bitmap_init(bitmap_t *out, allocator_t *allocator) {
    if (!allocator)
        allocator = libiter_allocator;

    if (out) {
        out->allocator = allocator;
        out->buffer = NULL;
        out->length = 0;
        out->capacity = 0;
    }

    return out;
}

bitmap_t *bitmap_create(allocator_t *allocator) {
    if (!allocator)
        allocator = libiter_allocator;

    bitmap_t *map = allocate(allocator, sizeof(*map));
    return bitmap_init(map, allocator);
}

void bitmap_destroy(bitmap_t *map) {
    bitmap_free(map);
    deallocate(map->allocator, map, sizeof(*map));
}

void bitmap_free(bitmap_t *map) { }

int bitmap_resize(bitmap_t *map, size_t capacity) {
    if (!map || capacity == 0)
        return ITER_EINVAL;

    capacity = PF_ALIGN_UP(capacity, BITMAP_INT_BITS);
    void *buf = reallocate(
        map->allocator,
        map->buffer,
        map->capacity / BITMAP_INT_BITS,
        capacity / BITMAP_INT_BITS
    );

    if (!buf)
        return ITER_ENOMEM;

    map->buffer = buf;
    map->capacity = capacity;
    if (map->length > capacity)
        map->length = capacity;
    return ITER_OK;
}

int bitmap_reserve(bitmap_t *map, size_t count) {
    if (!map || count == 0)
        return ITER_EINVAL;

    if (map->length + count <= map->capacity)
        return ITER_OK;

    return bitmap_resize(map, BITMAP_GROWTH(map->length, count));
}

static bitmap_int_t *bitmap_slot(bitmap_t *map, size_t *i) {
    bitmap_int_t *buf = map->buffer;
    bitmap_int_t *slot = &buf[(*i) / BITMAP_INT_BITS];

    *i = (*i) % BITMAP_INT_BITS;
    return slot;
}

int bitmap_get(bitmap_t *map, size_t i) {
    if (!map || i >= map->length)
        return ITER_EINVAL;

    bitmap_int_t *slot = bitmap_slot(map, &i);
    return *slot & BIT(1, i);
}

int bitmap_set(bitmap_t *map, size_t i, int value) {
    if (!map || i >= map->length)
        return ITER_EINVAL;

    bitmap_int_t *slot = bitmap_slot(map, &i);
    *slot |= BIT(value != 0, i);
    return ITER_OK;
}

int bitmap_toggle(bitmap_t *map, size_t i) {
    if (!map || i >= map->length)
        return ITER_EINVAL;

    bitmap_int_t *slot = bitmap_slot(map, &i);
    *slot ^= BIT(1, i);
    return ITER_OK;
}

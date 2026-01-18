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

#define PF_BITWISE_SKIP_DEFAULT
#include <pf_bitwise.h>
PF__IMPL_BITWISE(_bitmap, bitmap_int_t, BITMAP_INT_MAX);

#define BITMAP_GROWTH(old, count) (2 * ((old) + (count)))
#define BIT(value, offset) (((bitmap_int_t)value) << (offset))
#define MASK(count) ((((bitmap_int_t)1ULL) << (count)) - 1ULL)
#define SLOT_COUNT(length)                                   \
    (PF_ALIGN_UP(length, BITMAP_INT_BITS) / BITMAP_INT_BITS)
#define INCREMENT(i) PF_ALIGN_UP(i + 1, BITMAP_INT_BITS)

bitmap_t *bitmap_init(bitmap_t *out, allocator_t *allocator) {
    if (!allocator)
        allocator = libiter_allocator;

    if (out) {
        out->allocator = allocator;
        out->buffer = NULL;
        out->length = 0;
        out->as.capacity = 0;
    }

    return out;
}

bitmap_t *bitmap_create(allocator_t *allocator) {
    if (!allocator)
        allocator = libiter_allocator;

    bitmap_t *map = allocate(allocator, sizeof(*map));
    return bitmap_init(map, allocator);
}

int bitmap_slice(bitmap_t *dst, const bitmap_t *src, size_t from, size_t to) {
    if (!dst || !src)
        return ITER_EINVAL;

    if (to > src->length)
        to = src->length;
    if (from > to)
        from = to;

    dst->buffer = src->buffer;
    dst->length = to - from;
    dst->as.offset = from;
    dst->allocator = NULL;
    return ITER_OK;
}

void bitmap_destroy(bitmap_t *map) {
    bitmap_free(map);
    deallocate(map->allocator, map, sizeof(*map));
}

void bitmap_free(bitmap_t *map) {
    if (!map || !map->allocator)
        return;

    deallocate(map->allocator, map->buffer, map->as.capacity / BITMAP_INT_BITS);
}

int bitmap_resize(bitmap_t *map, size_t capacity) {
    if (!map || capacity == 0 || !map->allocator)
        return ITER_EINVAL;

    capacity = PF_ALIGN_UP(capacity, BITMAP_INT_BITS);
    void *buf = reallocate(
        map->allocator,
        map->buffer,
        map->as.capacity / BITMAP_INT_BITS,
        capacity / BITMAP_INT_BITS
    );

    if (!buf)
        return ITER_ENOMEM;

    map->buffer = buf;
    map->as.capacity = capacity;
    if (map->length > capacity)
        map->length = capacity;
    return ITER_OK;
}

int bitmap_reserve(bitmap_t *map, size_t count) {
    if (!map || count == 0 || !map->allocator)
        return ITER_EINVAL;

    if (map->length + count <= map->as.capacity)
        return ITER_OK;

    return bitmap_resize(map, BITMAP_GROWTH(map->length, count));
}

static bitmap_int_t bitmap_mask(size_t i, size_t len) {
    if (i >= BITMAP_INT_BITS)
        return 0;

    if (i + len > BITMAP_INT_BITS)
        len = BITMAP_INT_BITS - i;

    if (len == BITMAP_INT_BITS)
        return BITMAP_INT_MAX;

    return ((1ULL << len) - 1ULL) << i;
}

static bitmap_int_t *bitmap_slot(bitmap_t *map, size_t i, bitmap_int_t *mask) {
    if (!map->allocator)
        i += map->as.offset;

    if (i >= map->length)
        return NULL;

    bitmap_int_t *buf = map->buffer;
    bitmap_int_t *slot = &buf[i / BITMAP_INT_BITS];

    if (mask) {
        size_t j = i - i % BITMAP_INT_BITS;
        *mask = bitmap_mask(i - j, map->length - j);
    }
    return slot;
}

int bitmap_get(bitmap_t *map, size_t i) {
    if (!map)
        return ITER_EINVAL;

    bitmap_int_t *slot;
    if (!(slot = bitmap_slot(map, i, NULL)))
        return ITER_EINVAL;

    return *slot & BIT(1, i);
}

int bitmap_set(bitmap_t *map, size_t i, int value) {
    if (!map)
        return ITER_EINVAL;

    bitmap_int_t *slot;
    if (!(slot = bitmap_slot(map, i, NULL)))
        return ITER_EINVAL;

    *slot |= BIT(value != 0, i);
    return ITER_OK;
}

int bitmap_toggle(bitmap_t *map, size_t i) {
    if (!map)
        return ITER_EINVAL;

    bitmap_int_t *slot;
    if (!(slot = bitmap_slot(map, i, NULL)))
        return ITER_EINVAL;

    *slot ^= BIT(1, i);
    return ITER_OK;
}

int bitmap_inv(bitmap_t *map) {
    if (!map)
        return ITER_EINVAL;

    bitmap_int_t *buf, mask;

    for (size_t i = 0; i < map->length; i = INCREMENT(i)) {
        buf = bitmap_slot(map, i, &mask);
        *buf = (~(*buf) & mask) | (*buf & ~mask);
    }

    return ITER_OK;
}

#define BITMAP_BIN_OP(m_name, m_op)                          \
    int m_name(bitmap_t *dst, bitmap_t *src) {               \
        if (!dst || !src || dst->length != src->length)      \
            return ITER_EINVAL;                              \
                                                             \
        /* Binary operations are not supported for slices */ \
        if (!dst->allocator || !src->allocator)              \
            return ITER_ENOSYS;                              \
                                                             \
        bitmap_int_t *a, *b, mask;                           \
                                                             \
        for (size_t i = 0; i < dst->length; INCREMENT(i)) {  \
            a = bitmap_slot(src, i, &mask);                  \
            b = bitmap_slot(dst, i, NULL);                   \
            *b = ((*b m_op * a) & mask) | (*b & ~mask);      \
        }                                                    \
                                                             \
        return ITER_OK;                                      \
    }

BITMAP_BIN_OP(bitmap_or, |=);
BITMAP_BIN_OP(bitmap_and, &=);
BITMAP_BIN_OP(bitmap_xor, ^=);

int bitmap_shr(bitmap_t *map, size_t count) {
    if (!map)
        return ITER_EINVAL;

    if (count == 0)
        return ITER_OK;

    bitmap_int_t *buf, mask;

    for (size_t i = 0; i < map->length; i = INCREMENT(i)) {
        buf = bitmap_slot(map, i, &mask);
        *buf = ((*buf >> count) & mask) | (*buf & ~mask);
    }

    return ITER_OK;
}

int bitmap_shl(bitmap_t *map, size_t count) {
    if (!map)
        return ITER_EINVAL;

    if (count == 0)
        return ITER_OK;

    bitmap_int_t *buf, mask;

    for (size_t i = 0; i < map->length; i = INCREMENT(i)) {
        buf = bitmap_slot(map, i, &mask);
        *buf = ((*buf << count) & mask) | (*buf & ~mask);
    }

    return ITER_OK;
}

bitmap_int_t rotate_masked(bitmap_int_t value, bitmap_int_t mask, int count) {
    if (mask == 0 || count == 0)
        return value;

    if (mask == BITMAP_INT_MAX) {
        return count > 0 ? pf_rotl_bitmap(value, count)
                         : pf_rotr_bitmap(value, -count);
    }

    int bits = pf_popcount_bitmap(mask);

    /* Normalize shift to be within [0, bit_count) */
    int shift = PF_ABS(count) % bits;
    if (shift < 0)
        shift += bits;

    int start = pf_ctz_bitmap(mask);
    uint64_t normalized = (value & mask) >> start;

    uint64_t rotated = 0;
    if (count > 0) {
        rotated |= (normalized << shift);
        rotated |= (normalized >> (bits - shift));
    } else {
        rotated |= (normalized >> shift);
        rotated |= (normalized << (bits - shift));
    }

    if (bits != BITMAP_INT_BITS)
        rotated &= MASK(bits);

    return (value & ~mask) | (rotated << start);
}

int bitmap_rotl(bitmap_t *map, int count) {
    if (!map || count == INT_MIN)
        return ITER_EINVAL;

    if (count == 0)
        return ITER_OK;

    bitmap_int_t *buf, mask;

    for (size_t i = 0; i < map->length; i = INCREMENT(i)) {
        buf = bitmap_slot(map, i, &mask);
        *buf = rotate_masked(*buf, mask, count);
    }

    return ITER_OK;
}

int bitmap_rotr(bitmap_t *map, int count) {
    if (!map || count == INT_MIN)
        return ITER_EINVAL;

    if (count == 0)
        return ITER_OK;

    bitmap_int_t *buf, mask;

    for (size_t i = 0; i < map->length; i = INCREMENT(i)) {
        buf = bitmap_slot(map, i, &mask);
        *buf = rotate_masked(*buf, mask, -count);
    }

    return ITER_OK;
}

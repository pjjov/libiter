/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/global.h>
#include <iter/map.h>
#include <iter/types.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <allocator.h>
#include <allocator_joined.h>
#include <pf_macro.h>

#define PF_BITWISE_SKIP_DEFAULT
#include <pf_bitwise.h>
PF_IMPL_BITWISE(_size_t, size_t, SIZE_MAX);

#define MAP_GROWTH 2
#define MAP_DENSITY (3 / 4)

extern allocator_t *libiter_allocator;

static inline int can_heap_alloc(allocator_t *alloc) {
    return (((uintptr_t)alloc) & 1) == 0;
}

static inline int is_heap_alloc(map_t *m) {
    return (((uintptr_t)m->alloc) & 1) == 1;
}

static inline void mark_heap_alloc(map_t *out, allocator_t *alloc) {
    out->alloc = (void *)(((uintptr_t)alloc) | 1);
}

allocator_t *map_allocator(map_t *m) {
    if (!m)
        return NULL;

    uintptr_t mask = ~(uintptr_t)1;
    uintptr_t masked = ((uintptr_t)m->alloc) & mask;
    return (allocator_t *)(masked);
}

map_t *map_new(
    map_hash_fn *hash, allocator_t *alloc, size_t size, size_t align
) {
    if (!hash || size == 0 || !can_heap_alloc(alloc))
        return ITER_THROW_NULL(ITER_EINVAL);

    if (!alloc)
        alloc = libiter_allocator;

    map_t *out;

    if ((out = allocate(alloc, sizeof(*out))))
        return ITER_THROW_NULL(ITER_ENOMEM);

    map_init(out, hash, alloc, size, align);
    mark_heap_alloc(out, alloc);
    return out;
}

map_t *map_init(
    map_t *out, map_hash_fn *hash, allocator_t *alloc, size_t size, size_t align
) {
    if (!out || !hash || size == 0 || !can_heap_alloc(alloc))
        return ITER_THROW_NULL(ITER_EINVAL);

    if (!alloc)
        alloc = libiter_allocator;

    memset(out, 0, sizeof(*out));
    out->hash = hash;
    out->alloc = alloc;
    return out;
}

void map_free(map_t *m) {
    if (!m)
        return;

    allocator_t *alloc = map_allocator(m);

    deallocate(alloc, m->buffer, m->bufferSize);

    if (is_heap_alloc(m))
        deallocate(alloc, m, sizeof(*m));
}

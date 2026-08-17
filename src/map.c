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

enum {
    META_EMPTY = 0x00,
    META_USED = 0x01,
    META_DEAD = 0x02,
    STORAGE_EMPTY = 0x00,
    STORAGE_USED = 0x10,

    META_MASK = 0xF,
    STORAGE_MASK = 0xF0,
};

struct meta {
    uint8_t kind;
    uint8_t tag;
};

struct meta8 {
    struct meta base;
    uint8_t index;
};

struct meta16 {
    struct meta base;
    uint16_t index;
};

struct meta32 {
    struct meta base;
    uint32_t index;
};

struct meta64 {
    struct meta base;
    uint64_t index;
};

struct meta_iter {
    map_t *map;
    map_hash_fn *fn;

    const void *item;
    uint8_t tag;

    size_t mask;
    size_t slot;
};

static int meta_is_used(struct meta *m) {
    return (m->kind & META_MASK) == META_USED;
}

static int meta_is_empty(struct meta *m) {
    return (m->kind & META_MASK) == META_EMPTY;
}

static int meta_is_dead(struct meta *m) {
    return (m->kind & META_MASK) == META_DEAD;
}

static int meta_cmp_tag(struct meta *m, uint8_t tag) { return m->tag == tag; }

static int meta_cmp_item(struct meta_iter *iter, const void *item) {
    return 0 == iter->fn(iter->item, item);
}

static void meta_set_kind(struct meta *m, uint8_t kind) {
    m->kind = kind | (m->kind & STORAGE_MASK);
}

static void meta_set_tag(struct meta *m, uint8_t tag) { m->tag = tag; }

static int meta_stride(size_t cap) {
    if (cap <= UINT8_MAX)
        return sizeof(struct meta8);
    if (cap <= UINT16_MAX)
        return sizeof(struct meta16);
    if (cap <= UINT32_MAX)
        return sizeof(struct meta32);
    return sizeof(struct meta64);
}

static int meta_size(size_t cap) {
    if (cap <= UINT8_MAX)
        return sizeof(uint8_t);
    if (cap <= UINT16_MAX)
        return sizeof(uint16_t);
    if (cap <= UINT32_MAX)
        return sizeof(uint32_t);
    return sizeof(uint64_t);
}

static struct meta8 *meta_slot8(void *meta, size_t slot) {
    return &((struct meta8 *)meta)[slot];
}

static struct meta16 *meta_slot16(void *meta, size_t slot) {
    return &((struct meta16 *)meta)[slot];
}

static struct meta32 *meta_slot32(void *meta, size_t slot) {
    return &((struct meta32 *)meta)[slot];
}

static struct meta64 *meta_slot64(void *meta, size_t slot) {
    return &((struct meta64 *)meta)[slot];
}

static struct meta *meta_slot(map_t *map, size_t slot) {
    switch (meta_size(map->cap)) {
    case sizeof(uint8_t):
        return &meta_slot8(map->meta, slot)->base;
    case sizeof(uint16_t):
        return &meta_slot16(map->meta, slot)->base;
    case sizeof(uint32_t):
        return &meta_slot32(map->meta, slot)->base;
    case sizeof(uint64_t):
        return &meta_slot64(map->meta, slot)->base;
    default:
        PF_UNREACHABLE;
        return NULL;
    }
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

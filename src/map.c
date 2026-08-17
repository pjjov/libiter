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

#define TAG_MASK (sizeof(map_hash_t) * 8 - 8)

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

static int storage_is_used(struct meta *m) {
    return (m->kind & STORAGE_MASK) == STORAGE_USED;
}

static void storage_set_kind(struct meta *m, uint8_t kind) {
    m->kind = kind | (m->kind & META_MASK);
}

static size_t storage_reserve(map_t *map) { return map->storageTop; }

static void *storage_get(map_t *map, size_t index) {
    return PF_OFFSET(map->items, index * map->itemSize);
}

static void storage_set(map_t *map, const void *item, size_t index) {
    if (index == map->storageTop) {
        map->count++;
        map->storageTop++;
    }

    struct meta *m = meta_slot(map, index);
    storage_set_kind(m, STORAGE_USED);

    void *slot = storage_get(map, index);
    memcpy(slot, item, map->itemSize);
}

static void storage_remove(map_t *map, size_t index) {
    struct meta *m = meta_slot(map, index);
    storage_set_kind(m, STORAGE_EMPTY);
    map->count--;
}

static int storage_each(map_t *map, map_each_fn *fn, void *user) {
    size_t visited = 0;

    for (size_t i = 0; i < map->storageTop; i++) {
        if (visited >= map->count)
            break;

        struct meta *m = meta_slot(map, i);

        if (!storage_is_used(m))
            continue;

        void *slot = storage_get(map, i);

        if (fn(slot, user))
            return ITER_EINTR;

        visited++;
    }

    return ITER_OK;
}

static void meta_iter_init(
    struct meta_iter *iter, map_t *map, const void *item
) {
    map_hash_t hash = map->hash(item, NULL);

    iter->map = map;
    iter->fn = map->hash;
    iter->item = item;
    iter->mask = map->cap - 1;
    iter->slot = hash & iter->mask;
    iter->tag = (uint8_t)(hash >> TAG_MASK);
}

static void meta_iter_set(struct meta_iter *iter, size_t storageIndex) {
    struct meta *base;

    switch (meta_size(iter->map->cap)) {
    case sizeof(uint8_t): {
        struct meta8 *slot = meta_slot8(iter->map->meta, iter->slot);
        slot->index = storageIndex;
        base = &slot->base;
        break;
    }
    case sizeof(uint16_t): {
        struct meta16 *slot = meta_slot16(iter->map->meta, iter->slot);
        slot->index = storageIndex;
        base = &slot->base;
        break;
    }
    case sizeof(uint32_t): {
        struct meta32 *slot = meta_slot32(iter->map->meta, iter->slot);
        slot->index = storageIndex;
        base = &slot->base;
        break;
    }
    case sizeof(uint64_t): {
        struct meta64 *slot = meta_slot64(iter->map->meta, iter->slot);
        slot->index = storageIndex;
        base = &slot->base;
        break;
    }
    default:
        PF_UNREACHABLE;
        return;
    }

    meta_set_kind(base, META_USED);
    meta_set_tag(base, iter->tag);
}

static struct meta *meta_iter_get(
    struct meta_iter *iter, size_t *storageIndex
) {
    switch (meta_size(iter->map->cap)) {
    case sizeof(uint8_t): {
        struct meta8 *slot = &((struct meta8 *)iter->map->meta)[iter->slot];
        *storageIndex = slot->index;
        return &slot->base;
    }
    case sizeof(uint16_t): {
        struct meta16 *slot = &((struct meta16 *)iter->map->meta)[iter->slot];
        *storageIndex = slot->index;
        return &slot->base;
    }
    case sizeof(uint32_t): {
        struct meta32 *slot = &((struct meta32 *)iter->map->meta)[iter->slot];
        *storageIndex = slot->index;
        return &slot->base;
    }
    case sizeof(uint64_t): {
        struct meta64 *slot = &((struct meta64 *)iter->map->meta)[iter->slot];
        *storageIndex = slot->index;
        return &slot->base;
    }
    default:
        PF_UNREACHABLE;
        return NULL;
    }
}

static struct meta *meta_iter_next(
    struct meta_iter *iter, size_t *storageIndex
) {
    while (1) {
        struct meta *m = meta_iter_get(iter, storageIndex);

        if (meta_is_empty(m))
            return m;

        if (meta_is_dead(m)) {
            iter->slot = (iter->slot + 1) & iter->mask;
            continue;
        }

        void *item = storage_get(iter->map, *storageIndex);

        if (meta_cmp_tag(m, iter->tag) && meta_cmp_item(iter, item))
            return m;

        iter->slot = (iter->slot + 1) & iter->mask;
    }
}

static int meta_get(map_t *map, const void *item, size_t *storageIndex) {
    struct meta_iter iter;
    meta_iter_init(&iter, map, item);

    struct meta *m = meta_iter_next(&iter, storageIndex);
    return meta_is_empty(m) ? ITER_OK : ITER_ENOENT;
}

static int meta_set(
    map_t *map, const void *item, size_t *storageIndex, size_t newStorageIndex
) {
    struct meta_iter iter;
    meta_iter_init(&iter, map, item);

    struct meta *m = meta_iter_next(&iter, storageIndex);

    if (meta_is_empty(m))
        *storageIndex = newStorageIndex;

    meta_iter_set(&iter, *storageIndex);
    return ITER_OK;
}

static int meta_insert(map_t *map, const void *item, size_t newStorageIndex) {
    struct meta_iter iter;
    meta_iter_init(&iter, map, item);

    size_t storageIndex;
    struct meta *m = meta_iter_next(&iter, &storageIndex);

    if (!meta_is_empty(m))
        return ITER_EEXIST;

    meta_iter_set(&iter, newStorageIndex);
    return ITER_OK;
}

static int meta_update(map_t *map, const void *item, size_t *storageIndex) {
    struct meta_iter iter;
    meta_iter_init(&iter, map, item);

    struct meta *m = meta_iter_next(&iter, storageIndex);

    if (!meta_is_used(m))
        return ITER_ENOENT;

    meta_iter_set(&iter, *storageIndex);
    return ITER_OK;
}

static int meta_remove(map_t *map, const void *item, size_t *storageIndex) {
    struct meta_iter iter;
    meta_iter_init(&iter, map, item);

    struct meta *m = meta_iter_next(&iter, storageIndex);

    if (!meta_is_used(m))
        return ITER_ENOENT;

    meta_set_kind(m, META_DEAD);
    return ITER_OK;
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

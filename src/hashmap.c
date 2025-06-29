/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#define ITER_API
#include <iter/error.h>
#include <iter/hash.h>
#include <iter/hashmap.h>
#include <string.h>

#define META_SIZE 16
#define META_EMPTY 0
#define META_TOMB 1

#define HASHMAP_GROWTH 1.5
#define HASHMAP_THRESHOLD 0.7

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

extern allocator_t *libiter_allocator;

#if !defined(ITER_NO_SIMD) && defined(__SSE2__) && META_SIZE >= 16
    #define HASHMAP_SSE2
    #include <emmintrin.h>
#endif

/*
    Bucket structure is determined at runtime based on sizes and alignments of
    types. They are stored sequentially in the hashmap's buffer.

    ```c
    struct bucket {
        union hashmeta meta;
            padding
        K keys[META_SIZE];
            padding
        V values[META_SIZE];
            padding
    }

    map->koffset = offsetof(struct bucket, keys);
    map->voffset = offsetof(struct bucket, values);
    map->ksize = sizeof(K);
    map->vsize = sizeof(V);
    map->bucketSize = sizeof(struct bucket);
    ```
*/
union hashmeta {
    uint8_t parts[META_SIZE];
#ifdef HASHMAP_SSE2
    __m128i sse[META_SIZE / 16];
#endif
};

static inline uint64_t meta_match(union hashmeta *meta, uint8_t part) {
    uint64_t out = 0;
#ifdef __SSE2__
    __m128i tmp = _mm_set1_epi8(part);
    for (int i = 0; i < META_SIZE / 16; i++) {
        int result = _mm_movemask_epi8(_mm_cmpeq_epi8(meta->sse[i], tmp));
        out |= result << (i * 16);
    }
#else
    for (int i = 0; i < META_SIZE; i++) {
        if (meta->parts[i] == part)
            out |= 1 << i;
    }
#endif
    return out;
}

static inline uint8_t meta_part(hash_t hash) {
    uint8_t part = hash & 0xFF;
    if (part == META_EMPTY)
        part++;
    if (part == META_TOMB)
        part++;
    return part;
}

static inline int compare_key(
    const hashmap_t *map, const void *x, const void *y
) {
    return map->hash ? map->hash(x, y, map->hasher) : memcmp(x, y, map->ksize);
}

static inline uint64_t get_hash(const hashmap_t *map, const void *key) {
    return map->hash ? map->hash(key, NULL, map->hasher)
                     : map->hasher(key, map->ksize);
}

static inline union hashmeta *get_meta(const hashmap_t *map, size_t b) {
    return (union hashmeta *)((uintptr_t)map->buffer + map->bucketSize * b);
}

static inline void *get_key(
    const hashmap_t *map, union hashmeta *meta, uint8_t i
) {
    return (void *)((uintptr_t)meta + map->koffset + i * map->ksize);
}

static inline void *get_value(
    const hashmap_t *map, union hashmeta *meta, uint8_t i
) {
    return (void *)((uintptr_t)meta + map->voffset + i * map->vsize);
}

static inline size_t get_mask(const hashmap_t *map) {
    return (hashmap__capacity(map) - 1) / META_SIZE;
}

/* graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
static inline size_t round_pow2(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
    v |= v >> 32;
#endif
    v++;

    return v;
}

hashmap_t *hashmap__init(
    hashmap_t *out, allocator_t *allocator, const struct hashmap_layout *layout
) {
    if (!out || !layout || layout->ksize == 0)
        return NULL;

    if (!allocator)
        allocator = libiter_allocator;

    out->buffer = NULL;
    out->count = 0;

    out->ksize = layout->ksize;
    out->vsize = layout->vsize;

    size_t kalign = MAX(layout->kalign, layout->ksize);
    size_t valign = MAX(layout->valign, layout->vsize);

    /* Calculate padding to satisfy alignment requirements. */
    out->koffset = sizeof(union hashmeta);
    out->koffset += PF_ALIGN_PAD(sizeof(union hashmeta), kalign);
    out->voffset = out->koffset + layout->ksize * META_SIZE;
    out->voffset += PF_ALIGN_PAD(out->voffset, valign);
    out->bucketSize = out->voffset + layout->vsize * META_SIZE;
    out->bucketSize += PF_ALIGN_PAD(out->bucketSize, alignof(union hashmeta));

    out->hash = NULL;
    out->hasher = &hasher_fnv1a;
    out->allocator = allocator;
    return out;
}

hashmap_t *hashmap__create(
    allocator_t *allocator, const struct hashmap_layout *layout
) {
    if (!layout || layout->ksize == 0)
        return NULL;

    if (!allocator)
        allocator = libiter_allocator;

    hashmap_t *out = allocate(allocator, sizeof(hashmap_t));
    return hashmap__init(out, allocator, layout);
}

void hashmap__free(hashmap_t *map) {
    if (map) {
        size_t buckets = (1 << map->capacityLog2) / META_SIZE;
        deallocate(map->allocator, map->buffer, buckets * map->bucketSize);
    }
}

void hashmap__destroy(hashmap_t *map) {
    if (map) {
        hashmap__free(map);
        deallocate(map->allocator, map, sizeof(hashmap_t));
    }
}

int hashmap__use_hash(hashmap_t *map, hash_fn *hash, hasher_fn *hasher) {
    if (!map || map->count > 0)
        return ITER_EINVAL;

    map->hash = hash;
    map->hasher = hasher ? hasher : &hasher_fnv1a;
    return ITER_OK;
}

/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#define ITER_API
#include <iter/error.h>
#include <iter/hash.h>
#include <iter/hashmap.h>

#define META_SIZE 16
#define META_EMPTY 0
#define META_TOMB 1

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

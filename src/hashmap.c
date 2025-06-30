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
#define HASHMAP_MIN META_SIZE

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

hashmap_t *hashmap__with_capacity(
    size_t capacity, allocator_t *allocator, const struct hashmap_layout *layout
) {
    hashmap_t *out = hashmap__create(allocator, layout);

    if (out && capacity > 0 && hashmap__reserve(out, capacity)) {
        hashmap__destroy(out);
        return NULL;
    }

    return out;
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

int grow_empty(hashmap_t *map, size_t capacity) {
    void *buffer = reallocate(
        map->allocator,
        map->buffer,
        sizeof(union hashmeta) * capacity / META_SIZE,
        sizeof(union hashmeta) * capacity * 2 / META_SIZE
    );

    if (!buffer)
        return ITER_ENOMEM;

    map->buffer = buffer;
    map->capacityLog2 = sizeof(size_t) * 8 - __builtin_clzl(capacity);
    return ITER_OK;
}

int grow_not_empty(hashmap_t *map, size_t capacity) {
    hashmap_t tmp = *map;
    tmp.buffer = NULL;
    tmp.count = 0;

    if (!grow_empty(&tmp, capacity))
        return ITER_ENOMEM;

    for (size_t b = 0; b < hashmap__capacity(map) / META_SIZE; b++) {
        union hashmeta *meta = get_meta(map, b);

        for (uint8_t i = 0; i < META_SIZE; i++) {
            if (meta->parts[i] == META_EMPTY || meta->parts[i] == META_TOMB)
                continue;

            void *key = get_key(map, meta, i);
            void *value = get_value(map, meta, i);
            hashmap__fast_insert(&tmp, key, value);
        }
    }

    hashmap__free(map);
    *map = tmp;
    return ITER_OK;
}

int hashmap__reserve(hashmap_t *map, size_t count) {
    if (!map)
        return ITER_EINVAL;

    size_t capacity = hashmap__capacity(map);
    if (map->count + count <= capacity * HASHMAP_THRESHOLD)
        return ITER_OK;

    size_t required = hashmap__capacity(map);
    capacity = MAX(round_pow2(required), HASHMAP_MIN);

    if (map->count == 0)
        return grow_empty(map, capacity);
    return grow_not_empty(map, capacity);
}

#define BITSET_EACH(m_bitset, m_out)              \
    for (m_out = 0; (m_bitset) >> m_out; m_out++) \
        if ((m_bitset) & (1 << (m_out)))

#define BUCKET_EACH(m_hash, m_mask, m_out)                                 \
    for (m_out = (m_hash) & (m_mask);; (m_out) = ((m_out) + 1) & (m_mask))

void *hashmap__get(const hashmap_t *map, const void *key) {
    if (!map || !key || map->count == 0)
        return NULL;

    hash_t hash = get_hash(map, key);
    uint8_t i, part = meta_part(hash);
    size_t b, mask = get_mask(map);

    BUCKET_EACH(hash, mask, b) {
        union hashmeta *meta = get_meta(map, b);
        uint64_t matches = meta_match(meta, part);

        BITSET_EACH(matches, i) {
            if (0 == compare_key(map, key, get_key(map, meta, i)))
                return get_value(map, meta, i);
        }

        if (meta_match(meta, META_EMPTY))
            break;
    }

    return NULL;
}

int hashmap__set(hashmap_t *map, const void *key, const void *value) {
    if (!map || !key || !value)
        return ITER_EINVAL;

    if (hashmap__reserve(map, 1))
        return ITER_ENOMEM;

    hash_t hash = get_hash(map, key);
    uint8_t i, part = meta_part(hash);
    size_t b, mask = get_mask(map);

    BUCKET_EACH(hash, mask, b) {
        union hashmeta *meta = get_meta(map, b);
        uint64_t matches = meta_match(meta, part);

        BITSET_EACH(matches, i) {
            if (0 == compare_key(map, key, get_key(map, meta, i))) {
                memcpy(get_value(map, meta, i), value, map->vsize);
                return ITER_OK;
            }
        }

        matches = meta_match(meta, META_EMPTY);
        BITSET_EACH(matches, i) {
            map->count++;
            meta->parts[i] = part;
            memcpy(get_key(map, meta, i), key, map->ksize);
            memcpy(get_value(map, meta, i), value, map->vsize);
            return ITER_OK;
        }
    }
}

int hashmap__insert(hashmap_t *map, const void *key, const void *value) {
    if (!map || !key || !value)
        return ITER_EINVAL;

    if (hashmap__reserve(map, 1))
        return ITER_ENOMEM;

    hash_t hash = get_hash(map, key);
    uint8_t i, part = meta_part(hash);
    size_t b, mask = get_mask(map);

    BUCKET_EACH(hash, mask, b) {
        union hashmeta *meta = get_meta(map, b);
        uint64_t matches = meta_match(meta, part);

        BITSET_EACH(matches, i) {
            if (0 == compare_key(map, key, get_key(map, meta, i)))
                return ITER_EEXIST;
        }

        matches = meta_match(meta, META_EMPTY);
        BITSET_EACH(matches, i) {
            map->count++;
            meta->parts[i] = part;
            memcpy(get_key(map, meta, i), key, map->ksize);
            memcpy(get_value(map, meta, i), value, map->vsize);
            return ITER_OK;
        }
    }
}

int hashmap__remove(hashmap_t *map, const void *key) {
    if (!map || !key)
        return ITER_EINVAL;

    if (map->count == 0)
        return ITER_ENOENT;

    hash_t hash = get_hash(map, key);
    uint8_t i, part = meta_part(hash);
    size_t b, mask = get_mask(map);

    BUCKET_EACH(hash, mask, b) {
        union hashmeta *meta = get_meta(map, b);
        uint64_t matches = meta_match(meta, part);

        BITSET_EACH(matches, i) {
            if (0 == compare_key(map, key, get_key(map, meta, i))) {
                meta->parts[i] = META_TOMB;
                map->count--;
                return ITER_OK;
            }
        }

        if (meta_match(meta, META_EMPTY))
            break;
    }

    return ITER_ENOENT;
}

int hashmap__fast_insert(hashmap_t *map, const void *key, const void *value) {
    if (!map || !key || !value)
        return ITER_EINVAL;

    if (hashmap__reserve(map, 1))
        return ITER_ENOMEM;

    hash_t hash = get_hash(map, key);
    uint8_t i, part = meta_part(hash);
    size_t b, mask = get_mask(map);

    BUCKET_EACH(hash, mask, b) {
        union hashmeta *meta = get_meta(map, b);
        uint64_t matches = meta_match(meta, META_EMPTY);

        BITSET_EACH(matches, i) {
            map->count++;
            meta->parts[i] = part;
            memcpy(get_key(map, meta, i), key, map->ksize);
            memcpy(get_value(map, meta, i), value, map->vsize);
            return ITER_OK;
        }
    }
}

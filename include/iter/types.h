/*  libiter - Generic container and iterator library for C.

    Copyright 2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_TYPES_H

    #ifndef LIBITER_TYPES_ONCE
        #define LIBITER_TYPES_ONCE
        #include <iter/generic.h>
        #include <stddef.h>

typedef struct allocator_t allocator_t;

        #ifndef ITER_HASH_TYPE
            #include <limits.h>
            #define ITER_HASH_TYPE size_t
            #define HASH_MIN SIZE_MIN
            #define HASH_MAX SIZE_MAX
        #endif

typedef ITER_HASH_TYPE hash_t;
typedef hash_t(hasher_fn)(const void *buffer, size_t length);
typedef hash_t(hash_fn)(const void *item, const void *other, hasher_fn *hasher);

    #endif

    #ifndef LIBITER_NEED_TYPE
        #define LIBITER_NEED_BITMAP
        #define LIBITER_NEED_HASHMAP
        #define LIBITER_NEED_ITER
        #define LIBITER_NEED_POOL
        #define LIBITER_NEED_VECTOR
    #else
        #undef LIBITER_NEED_TYPE
    #endif

    #if defined(LIBITER_NEED_BITMAP) && !defined(LIBITER_HAS_BITMAP)

        #define LIBITER_HAS_BITMAP

typedef struct bitmap_t {
    size_t length;
    void *buffer;
    allocator_t *allocator;

    union {
        size_t capacity;
        size_t offset;
    } as;
} bitmap_t;

    #endif

    #if defined(LIBITER_NEED_HASHMAP) && !defined(LIBITER_HAS_HASHMAP)

        #define LIBITER_HAS_HASHMAP
        #define hashmap(K, V) generic_container(hashmap_t, K, V)

typedef struct hashmap_t {
    void *buffer;
    size_t count;

    unsigned int ksize;
    unsigned int koffset;
    unsigned int vsize;
    unsigned int voffset;
    unsigned int bucketSize;
    unsigned int capacityLog2;

    hash_fn *hash;
    hasher_fn *hasher;
    allocator_t *allocator;
} hashmap_t;

    #endif

    #if defined(LIBITER_NEED_POOL) && !defined(LIBITER_HAS_POOL)

        #define LIBITER_HAS_POOL
        #define pool(T) generic_container(pool_t, size_t, T)

typedef struct pool_t {
    void *buffer;
    size_t size;
    size_t align;
    size_t count;
    size_t capacity;
    allocator_t *allocator;
} pool_t;

    #endif

    #if defined(LIBITER_NEED_VECTOR) && !defined(LIBITER_HAS_VECTOR)

        #define LIBITER_HAS_VECTOR
        #define vector(T) generic_container(vector_t, size_t, T)

typedef struct vector_t {
    void *items;
    size_t length;
    size_t capacity;
    allocator_t *allocator;
} vector_t;

    #endif

    #if defined(LIBITER_HAS_POOL) && defined(LIBITER_HAS_VECTOR)

        #define LIBITER_TYPES_H
    #endif

#endif

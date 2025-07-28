/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_HASH_H
#define LIBITER_HASH_H

/** # Hashing functions

    We divide hashing into two functions: `hash_fn` and `hasher_fn`.

    `hasher_fn` is the function that takes in a buffer of bytes
    and outputs a hash.

    `hash_fn` uses the hasher to hash a specific type, ignoring
    padding and other irrelevant data. You can also use `hash_fn`
    to provide a cached hash value.

    Hashing functions are used for associative containers and they
    heavily impact their perfomance. Picking a right function depends
    on the data used and thorough benchmarking should be done.
**/

#include <stddef.h>
#include <stdint.h>

#ifndef ITER_API
    #define ITER_API static inline
#endif

#ifndef ITER_HASH_TYPE
    #include <limits.h>
    #define ITER_HASH_TYPE size_t
    #define HASH_MIN SIZE_MIN
    #define HASH_MAX SIZE_MAX
#endif

/** ## HASH_BITS

    Length of `hash_t` in bits, calculated from HASH_MAX.
**/
#if HASH_MAX > 0xFFFFFFFFFFFFFFFFul
    #define HASH_BITS 128
#elif HASH_MAX > 0xFFFFFFFFul
    #define HASH_BITS 64
#elif HASH_MAX > 0xFFFFul
    #define HASH_BITS 32
#elif HASH_MAX > 0xFFul
    #define HASH_BITS 16
#else
    #define HASH_BITS 8
#endif

/** ## hash_t

    `hash_t` is the integer type used for storing hash values, by default
    identical to `size_t`, which can be changed using `ITER_HASH_TYPE`.
**/
typedef ITER_HASH_TYPE hash_t;

/** ## hasher_fn

    This function callback takes a `buffer` of bytes of size `length`
    and outputs a corresponding hash value. If two items are equal,
    the callback should return the same hash value for both.
**/
typedef hash_t(hasher_fn)(const void *buffer, size_t length);

/** ## hash_fn

    If `other` is `NULL`, this function callback generates a hash value
    for an item using the provided `hasher_fn`.

    If `other` is not `NULL`, this function callback should compare `other`
    and `item` for equality, returning 0 if they are equal and a non-zero
    value otherwise.

    The callback should ignore padding and other irrelevant data
    when producing a hash. If two items are equal, the callback
    should return the same hash value for both.
**/
typedef hash_t(hash_fn)(const void *item, const void *other, hasher_fn *hasher);

#if !defined(HASH_FNV_PRIME) || !defined(HASH_FNV_OFFSET)
    #if HASH_BITS == 128
        #define HASH_FNV_PRIME 0x0000000001000000000000000000013bull
        #define HASH_FNV_OFFSET 0x6c62272e07bb014262b821756295c58dull
    #elif HASH_BITS == 64
        #define HASH_FNV_PRIME 0x00000100000001b3ull
        #define HASH_FNV_OFFSET 0xcbf29ce484222325ull
    #elif HASH_BITS == 32
        #define HASH_FNV_PRIME 0x01000193ull
        #define HASH_FNV_OFFSET 0x811c9dc5ull
    #endif
#endif

#if defined(HASH_FNV_PRIME) && defined(HASH_FNV_OFFSET)

/** hash_t hasher_fnv1a(const void *buffer, size_t length)

    Fowler–Noll–Vo hash function variant for the `hasher_fn` interface.
    You can define HASH_FNV_PRIME and HASH_FNV_OFFSET to change the parameters.
    Source: en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
**/
ITER_API hash_t hasher_fnv1a(const void *buffer, size_t length) {
    hash_t hash = HASH_FNV_OFFSET;
    const char *bytes = buffer;

    while (length--) {
        hash ^= (hash_t)(unsigned char)*bytes++;
        hash *= HASH_FNV_PRIME;
    }
    return hash;
}

#endif

/** hash_t hasher_djb2(const void *buffer, size_t length)

    Hash function by Dan Bernstein.
    Source: www.cse.yorku.ca/~oz/hash.html
**/
ITER_API hash_t hasher_djb2(const void *buffer, size_t length) {
    uint32_t hash = 5381;
    const char *bytes = buffer;

    while (length--)
        hash = ((hash << 5) + hash) + *bytes++; /* hash * 33 + c */

    return hash;
}

/** hash_t hasher_sdbm(const void *buffer, size_t length)

    Hash function created for sdbm (a public-domain reimplementation of ndbm).
    Source: www.cse.yorku.ca/~oz/hash.html
**/
ITER_API hash_t hasher_sdbm(const void *buffer, size_t length) {
    uint32_t hash = 0;
    const char *bytes = buffer;

    while (length--)
        hash = *bytes++ + (hash << 6) + (hash << 16) - hash;

    return hash;
}

/** hash_t hasher_elf(const void *buffer, size_t length)

    Hash function by Peter J. Weinberger.
    Source: en.wikipedia.org/wiki/PJW_hash_function
**/
ITER_API hash_t hasher_elf(const void *buffer, size_t length) {
    const unsigned char *bytes = buffer;
    uint32_t h = 0, high;

    while (*bytes) {
        h = (h << 4) + *bytes++;
        if ((high = h & 0xF0000000))
            h ^= high >> 24;
        h &= ~high;
    }

    return h;
}

#endif

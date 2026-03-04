/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef ITER_BITMAP_H
#define ITER_BITMAP_H

#ifndef ITER_API
    #define ITER_API
#endif

#ifndef ITER_INLINE
    #define ITER_INLINE static inline
#endif

#include <stddef.h>

typedef struct allocator_t allocator_t;

enum {
    BITMAP_CLEAR = 0,
    BITMAP_SET = 1,
};

typedef struct bitmap_t {
    size_t length;
    void *buffer;
    allocator_t *allocator;

    union {
        size_t capacity;
        size_t offset;
    } as;
} bitmap_t;

/** Initializes `out` and returns it. With this function, you can allocate
    the bitmap object onto the stack. If `allocator` is `NULL`, the default
    one will be used instead. Returns `NULL` if unsuccessful.

    > You should use `bitmap_free` to free the returned bitmap's resources
    > instead of `bitmap_destroy` when this function is used.
**/
ITER_API bitmap_t *bitmap_init(bitmap_t *out, allocator_t *allocator);

/** Creates a new instance of `bitmap_t`, allocated with `allocator`.
    Returns `NULL` if out of memory.
**/
ITER_API bitmap_t *bitmap_create(allocator_t *allocator);

/** Initializes `dst` as a slice of bits from `src`, starting at index
    `from` (inclusive) and ending at index `to` (exclusive).

    Possible error codes: ITER_EINVAL.
**/
ITER_API int bitmap_slice(
    bitmap_t *dst, const bitmap_t *src, size_t from, size_t to
);

/** Frees all resources used by `map` if it's not `NULL`. **/
ITER_API void bitmap_destroy(bitmap_t *map);

/** Frees all resources used by `map`, which was initialized
    by `bitmap_init` beforehand, if it's not `NULL`.
**/
ITER_API void bitmap_free(bitmap_t *map);

/** Returns the number of bits present in `map` or 0 if `map` is `NULL` **/
ITER_INLINE size_t bitmap_length(bitmap_t *map) {
    return map ? map->length : 0;
}

/** Returns the number of slots reserved in `map` or 0 if `map` is `NULL` **/
ITER_INLINE size_t bitmap_capacity(bitmap_t *map) {
    if (!map)
        return 0;

    return map->allocator ? map->as.capacity : map->length;
}

/** Returns the offset of `map` if it's a slice, 0 otherwise. **/
ITER_INLINE size_t bitmap_offset(bitmap_t *map) {
    if (!map || map->allocator)
        return 0;
    return map->as.offset;
}

/** Returns the allocator used by `map`, or `NULL` if it's a slice. **/
ITER_INLINE allocator_t *bitmap_allocator(bitmap_t *map) {
    return map ? map->allocator : NULL;
}

/** Reserves space for `count` more bits in `map`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
ITER_API int bitmap_reserve(bitmap_t *map, size_t count);

/** Resizes `map` to fit exactly `length` characters.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
ITER_API int bitmap_resize(bitmap_t *map, size_t length);

/** Returns the boolean value of the bit at index `i`.
    Possible error codes: ITER_EINVAL.
**/
ITER_API int bitmap_get(bitmap_t *map, size_t i);

/** Sets the boolean value of the bit at index `i`.
    Possible error codes: ITER_EINVAL.
**/
ITER_API int bitmap_set(bitmap_t *map, size_t i, int value);

/** Returns the boolean value of the bit at index `i` and toggles it.
    Possible error codes: ITER_EINVAL.
**/
ITER_API int bitmap_toggle(bitmap_t *map, size_t i);

/** Copies `count` bits starting at index `i` into `out`, packed as bytes.
    Possible error codes: ITER_EINVAL.
**/
ITER_API int bitmap_getn(bitmap_t *map, char *out, size_t i, size_t count);

/** Sets the value of `count` bits starting at index `i` to `value`.
    Possible error codes: ITER_EINVAL.
**/
ITER_API int bitmap_setn(bitmap_t *map, int value, size_t i, size_t count);

/** Sets all bits of `map` to 0.
    Possible error codes: ITER_EINVAL.
**/
ITER_INLINE int bitmap_clear(bitmap_t *map) {
    return bitmap_setn(map, 0, 0, bitmap_length(map));
}

/** Toggles the value of `count` bits starting at index `i`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
ITER_API int bitmap_togglen(bitmap_t *map, size_t i, size_t count);

/** Inverts the value of all bits in `map`.
    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_inv(bitmap_t *map);

/** Performs the OR operation between bits at each index
    of `dst` and `src` and stores results in `dst`.
    The length of both `dst` and `src` has to be equal.

    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_or(bitmap_t *dst, bitmap_t *src);

/** Performs the AND operation between bits at each index
    of `dst` and `src` and stores results in `dst`.
    The length of both `dst` and `src` has to be equal.

    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_and(bitmap_t *dst, bitmap_t *src);

/** Performs the XOR operation between bits at each index
    of `dst` and `src` and stores results in `dst`.
    The length of both `dst` and `src` has to be equal.

    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_xor(bitmap_t *dst, bitmap_t *src);

/** Shifts all bits from `map` to the right by `count`.
    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_shr(bitmap_t *map, size_t count);

/** Shifts all bits from `map` to the left by `count`.
    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_shl(bitmap_t *map, size_t count);

/** Rotates all bits from `map` to the right by `count`.
    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_rotr(bitmap_t *map, int count);

/** Rotates all bits from `map` to the left by `count`.
    Possible error codes: ITER_EINVAL
**/
ITER_API int bitmap_rotl(bitmap_t *map, int count);

/** Returns the number of trailing zero bits in `map`. **/
ITER_API size_t bitmap_ctz(bitmap_t *map);

/** Returns the number of leading zero bits in `map`. **/
ITER_API size_t bitmap_clz(bitmap_t *map);

/** Returns the number of trailing one bits in `map`. **/
ITER_API size_t bitmap_cto(bitmap_t *map);

/** Returns the number of leading one bits in `map`. **/
ITER_API size_t bitmap_clo(bitmap_t *map);

/** Returns the index of first trailing zero in `map`. **/
ITER_API size_t bitmap_ftz(bitmap_t *map);

/** Returns the index of first leading zero in `map`. **/
ITER_API size_t bitmap_flz(bitmap_t *map);

/** Returns the index of first trailing one in `map`. **/
ITER_API size_t bitmap_fto(bitmap_t *map);

/** Returns the index of first leading one in `map`. **/
ITER_API size_t bitmap_flo(bitmap_t *map);

/** Returns the number of one bits in `map`. **/
ITER_API size_t bitmap_popcount(bitmap_t *map);

/** Returns the number of zero bits in `map`. **/
ITER_API size_t bitmap_zerocount(bitmap_t *map);

/** Returns the parity of bits in `map`. **/
ITER_API int bitmap_parity(bitmap_t *map);

#endif

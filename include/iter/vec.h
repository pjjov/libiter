/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_VEC_H
#define LIBITER_VEC_H

#define LIBITER_NEED_TYPE
#define LIBITER_NEED_VEC
#include <iter/types.h>

#ifndef ITER_API
    #define ITER_API
#endif

#ifndef ITER_INLINE
    #define ITER_INLINE static inline
#endif

/** Group: Helper macros */

/** Extracts the item type for a given vector. */
#define vec_type(VEC) generic_item_type(vec_t, VEC)

/** Extracts the item type as a pointer for a given vector. */
#define vec_type_ptr(VEC) generic_item_ptr(vec_t, VEC)

/** Extracts the size of the item type for a given vector. */
#define vec_type_size(VEC) generic_item_size(vec_t, VEC)

/** Cast the vector into `vec_t *` with some type safety. */
#define vec_base(VEC) generic_check_container(vec_t, VEC)

/** Checks if given item has the vector's subtype. */
#define vec_check(VEC, ITEM) generic_check_item(vec_t, VEC, ITEM)

/** Group: Allocation functions
    Custom-prototype: true
*/

/** vec(T) vecnew(type T, allocator_t *allocator);

    Creates a new instance of `vec(T)`, allocated with 'allocator'.
    Returns: `NULL` if out of memory.
*/
#define vecnew(T, allocator) ((vec(T))vec_new((allocator)))

/** vec(T) vecinit(type T, vec_t *out, allocator_t *allocator);

    Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
*/
#define vecinit(T, out, allocator) ((vec(T))vec_new((out), (allocator)))

/** T *vecunwrap(vec(T) v);

    Returns the item buffer of 'v' and frees 'v' if `vecinit` was not used.
*/
#define vecunwrap(v) vec_unwrap(vec_base(v))

/** void vecfree(vec(T) v);

    Frees the item buffer of 'v' and frees 'v' if `vecinit` was not used.
*/
#define vecfree(v) vec_free(vec_base(v))

/** Group: Properties */

/** size_t veclen(vec(T) v)

    Returns the number of items present in 'v'.
*/
#define veclen(v) (vec_len(vec_base(v), vec_type_size(v)))

/** size_t veccap(vec(T) v)

    Returns the number of slots allocated in 'v'.
*/
#define veccap(v) (vec_cap(vec_base(v), vec_type_size(v)))

/** allocator_t *vecallocator(vec(T) v)

    Returns the allocator used by 'v'.
*/
#define vecallocator(v) vec_allocator(vec_base(v))

/** Group: Typeless functions
    Custom-prototype: false
*/

/** Creates a new instance of `vec_t`. */
ITER_API vec_t *vec_new(allocator_t *allocator);

/** Initializes 'out' and returns it. */
ITER_API vec_t *vec_init(vec_t *out, allocator_t *allocator);

/** Returns the item buffer and frees 'v'. */
ITER_API void *vec_unwrap(vec_t *v);

/** Frees the item buffer and vector 'v'. */
ITER_API void vec_free(vec_t *v);

/** Returns the number of bytes used in 'v'. */
ITER_INLINE size_t vec_len(vec_t *v, size_t size) {
    return v || size == 0 ? v->len / size : 0;
}

/** Returns the number of bytes allocated in 'v'. */
ITER_INLINE size_t vec_cap(vec_t *v, size_t size) {
    return v || size == 0 ? v->cap / size : 0;
}

/** Returns allocator used by 'v'. */
ITER_API allocator_t *vec_allocator(vec_t *v);

#endif
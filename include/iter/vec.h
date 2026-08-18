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

/** Checks if given item has the vector's subtype as double pointer. */
#define vec_checkd(VEC, ITEM) generic_check_item_d(vec_t, VEC, ITEM)

/** Expands into a for loop body with the given variable name. */
#define VECFORBODY(VEC, VAR)               \
    vec_type_ptr(VEC) VAR = vecstart(VEC); \
    VAR < vecend(VEC);                     \
    VAR++

/** Expands into a for loop with the given variable name. */
#define VECFOREACH(VEC, VAR) for (VECFORBODY(VEC, VAR))

/** Group: Allocation functions
    Custom-prototype: true
*/

/** vec(T) vecnew(type T, allocator_t *allocator);

    Creates a new instance of `vec(T)`, allocated with 'allocator'.
    Returns: `NULL` if out of memory.
*/
#define vecnew(T, allocator) ((vec(T))vec_new((allocator)))

/** vec(T) vecnew_array(T *array, size_t length, allocator_t *allocator);

    Creates a new instance of `vec(T)`, allocated with 'allocator'.
    The content of 'array' is copied to the resulting vector.
*/
#define vecnew_array(array, length, allocator)                          \
    ((vec(typeof(*array)))                                              \
         vec_new_array((array), (length), (allocator), sizeof(*array)))

/** vec(T) vecnew_arrayp(T **array, size_t length, allocator_t *allocator);

    Creates a new instance of `vec(T)`, allocated with 'allocator'.
    The content of 'array' is copied to the resulting vector.
*/
#define vecnew_arrayp(array, length, allocator)                           \
    ((vec(typeof(**array)))                                               \
         vec_new_arrayp((array), (length), (allocator), sizeof(**array)))

/** vec(T) vecinit(type T, vec_t *out, allocator_t *allocator);

    Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
*/
#define vecinit(T, out, allocator) ((vec(T))vec_init((out), (allocator)))

/** vec(T) vecinit_array(T *array, size_t length, vec_t *out, allocator_t *allocator);

    Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
    The content of 'array' is copied to the resulting vector.
*/
#define vecinit_array(array, length, out, allocator)          \
    ((vec(typeof(*array)))vec_init_array(                     \
        (array), (length), (out), (allocator), sizeof(*array) \
    ))

/** vec(T) vecinit_arrayp(T **array, size_t length, vec_t *out, allocator_t *allocator);

    Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
    The content of 'array' is copied to the resulting vector.
*/
#define vecinit_arrayp(array, length, out, allocator)          \
    ((vec(typeof(**array)))vec_init_arrayp(                    \
        (array), (length), (out), (allocator), sizeof(**array) \
    ))

/** T *vecunwrap(vec(T) v);

    Returns the item buffer of 'v' and frees 'v' if `vecinit` was not used.
*/
#define vecunwrap(v) vec_unwrap(vec_base(v))

/** void vecfree(vec(T) v);

    Frees the item buffer of 'v' and frees 'v' if `vecinit` was not used.
*/
#define vecfree(v) vec_free(vec_base(v))

/** int vecresize(vec(T) v, size_t cap);

    Resizes the item buffer to fit 'cap' items.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecresize(v, cap) (vec_resize(vec_base(v), (cap), vec_type_size(v)))

/** int vecreserve(vec(T) v, size_t count);

    Reserves space for at least 'count' items.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecreserve(v, count)                              \
    (vec_reserve(vec_base(v), (count), vec_type_size(v)))

/** Group: Properties */

/** size_t veclen(vec(T) v);

    Returns the number of items present in 'v'.
*/
#define veclen(v) (vec_len(vec_base(v), vec_type_size(v)))

/** size_t veccap(vec(T) v);

    Returns the number of slots allocated in 'v'.
*/
#define veccap(v) (vec_cap(vec_base(v), vec_type_size(v)))

/** allocator_t *vecallocator(vec(T) v);

    Returns the allocator used by 'v'.
*/
#define vecallocator(v) vec_allocator(vec_base(v))

/** T *vecslot(vec(T) v, size_t i);

    Returns a pointer to the item slot at index i.
*/
#define vecslot(v, i) (vec_slot(vec_base(v), (i), vec_type_size(v)))

/** T *vecptr(vec(T) v, size_t i);

    Returns a pointer to the item at index i.
*/
#define vecptr(v, i) (vec_ptr(vec_base(v), (i), vec_type_size(v)))

/** T vecget(vec(T) v, size_t i);

    Returns the item at index i.
*/
#define vecget(v, i) (*vec_ptr(vec_base(v), (i), vec_type_size(v)))

/** size_t vecindex(vec(T) v, T *item);

    Returns the index of item at given pointer.
    Throws: ITER_EINVAL.
*/
#define vecindex(v, item) (vec_index(vec_base(v), (item), vec_type_size(v)))

/** T *vecitems(vec(T) v);

    Returns the pointer of the allocated item buffer.
*/
#define vecitems(v) (vec_items(vec_base(v)))

/** T *vecstart(vec(T) v);

    Returns a pointer to the start of the used part of the item buffer.
*/
#define vecstart(v) (vec_start(vec_base(v)))

/** T *vecend(vec(T) v);

    Returns a pointer to the end of the used part of the item buffer.
*/
#define vecend(v) (vec_end(vec_base(v)))

/** int vecisempty(vec(T) v);

    Checks if length of 'v' is 0.
*/
#define vecisempty(v) (vec_isempty(vec_base(v)))

/** void vecclear(vec(T) v);

    Sets the length of 'v' to 0.
*/
#define vecclear(v) (vec_clear(vec_base(v)))

/** void vecsetlen(vec(T) v, size_t len);

    Sets the length of 'v' to given parameter.
*/
#define vecsetlen(v, len) (vec_setlen(vec_base(v), (len), vec_type_size(v)))

/** void vecsetcap(vec(T) v, size_t cap);

    Sets the capacity of 'v' to given parameter.
*/
#define vecsetcap(v, cap) (vec_setcap(vec_base(v), (cap), vec_type_size(v)))

/** Group: Main operations */

/** int vecinsert(vec(T) v, T *items, size_t i, size_t count);

    Inserts 'count' items at index 'i'.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecinsert(v, items, i, count) \
    vec_insert(vec_base(v), vec_check(v, items), (i), (count), vec_type_size(v))

/** int vecpush(vec(T) v, T *items, size_t count);

    Inserts 'count' items at the end of the item buffer.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecpush(v, items, count)                                          \
    vec_push(vec_base(v), vec_check(v, items), (count), vec_type_size(v))

/** int vecunshift(vec(T) v, T *items, size_t count);

    Inserts 'count' items at the start of the item buffer.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecunshift(v, items, count)                                          \
    vec_unshift(vec_base(v), vec_check(v, items), (count), vec_type_size(v))

/** int vecinsertp(vec(T) v, T **items, size_t i, size_t count);

    Inserts 'count' items at index 'i', by pointer.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecinsertp(v, items, i, count)                                    \
    vec_insertp(                                                          \
        vec_base(v), vec_checkd(v, items), (i), (count), vec_type_size(v) \
    )

/** int vecfill(vec(T) v, T *item, size_t i, size_t count);

    Inserts 'count' copies of 'item' at index 'i'.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecfill(v, item, i, count)                                            \
    vec_fill(vec_base(v), vec_check(v, item), (i), (count), vec_type_size(v))

/** int vecremove(vec(T) v, T *out, size_t i, size_t count);

    Removes 'count' items start at index 'i'.
    If 'out' is not `NULL`, removed items are copied to it.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecremove(v, out, i, count)                                \
    vec_remove(vec_base(v), (out), (i), (count), vec_type_size(v))

/** int vecpop(vec(T) v, T *out, size_t count);

    Removes 'count' items from the end of the item buffer.
    If 'out' is not `NULL`, removed items are copied to it.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecpop(v, out, count)                              \
    vec_pop(vec_base(v), (out), (count), vec_type_size(v))

/** int vecshift(vec(T) v, T *out, size_t count);

    Removes 'count' items from the start of the item buffer.
    If 'out' is not `NULL`, removed items are copied to it.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecshift(v, out, count)                              \
    vec_shift(vec_base(v), (out), (count), vec_type_size(v))

/** int vecswap(vec(T) v, size_t i, size_t j, size_t count);

    Swaps 'count' items starting at indices 'i' and 'j'.
    > Space past the last item is used as a temporary buffer.
    Throws: ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.
*/
#define vecswap(v, i, j, count)                                \
    vec_swap(vec_base(v), (i), (j), (count), vec_type_size(v))

/** int vecswap_remove(vec(T) v, T *out, size_t i, size_t count);

    Removes 'count' items start at index 'i', by swapping items.
    If 'out' is not `NULL`, removed items are copied to it.
    Throws: ITER_EINVAL, ITER_EOVERFLOW.
*/
#define vecswap_remove(v, out, i, count)                                \
    vec_swap_remove(vec_base(v), (out), (i), (count), vec_type_size(v))

/** Group: Iteration functions */

/** int veceach(vec(T) v, vec_each_fn *fn, void *user);

    Calls 'fn' for each item in 'v'. If the callback returns a non-zero value,
    the iteration is interrupted and the function exits immediately.

    Returns: ITER_EINVAL, ITER_EINTR.
    Throws: ITER_EINVAL.
*/
#define veceach(v, fn, user)                              \
    vec_each(vec_base(v), (fn), (user), vec_type_size(v))

/** Group: Typeless functions
    Custom-prototype: false
*/

#ifndef vec__mul
    #define vec__mul(a, b) ((b) > 0 && (a) > SIZE_MAX / (b))
#endif

/** Creates a new instance of `vec_t`. */
ITER_API vec_t *vec_new(allocator_t *allocator);

/** Creates a new instance of `vec_t`. Copies the content of 'array'. */
ITER_API vec_t *vec_new_array(
    void *array, size_t length, allocator_t *allocator, size_t size
);

/** Creates a new instance of `vec_t`. Copies the content of 'array'. */
ITER_API vec_t *vec_new_arrayp(
    void **array, size_t length, allocator_t *allocator, size_t size
);

/** Initializes 'out' and returns it. */
ITER_API vec_t *vec_init(vec_t *out, allocator_t *allocator);

/** Initializes 'out' and returns it. Copies the content of 'array'. */
ITER_API vec_t *vec_init_array(
    void *array, size_t length, vec_t *out, allocator_t *allocator, size_t size
);

/** Initializes 'out' and returns it. Copies the content of 'array'. */
ITER_API vec_t *vec_init_arrayp(
    void **array, size_t length, vec_t *out, allocator_t *allocator, size_t size
);

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

/** Returns pointer to the item slot. */
ITER_INLINE void *vec_slot(vec_t *v, size_t i, size_t size) {
    if (!v || vec__mul(i, size))
        return NULL;

    size_t off = i * size;
    unsigned char *buf = v->items;
    return off < v->cap ? &buf[off] : NULL;
}

/** Returns pointer to the item at given index. */
ITER_INLINE void *vec_ptr(vec_t *v, size_t i, size_t size) {
    if (!v || vec__mul(i, size))
        return NULL;

    size_t off = i * size;
    unsigned char *buf = v->items;
    return off < v->len ? &buf[off] : NULL;
}

/** Returns the item index of given pointer. */
ITER_API size_t vec_index(vec_t *v, void *item, size_t size);

/** Returns the item buffer. */
ITER_INLINE void *vec_items(vec_t *v) { return v ? v->items : NULL; }

/** Returns the pointer past the first item. */
ITER_INLINE void *vec_start(vec_t *v) {
    if (!v || v->len == 0)
        return NULL;
    return v->items;
}

/** Returns the pointer past the last item. */
ITER_INLINE void *vec_end(vec_t *v) {
    if (!v || v->len == 0)
        return NULL;
    unsigned char *buf = v->items;
    return &buf[v->len];
}

/** Checks if length is 0. */
ITER_INLINE int vec_isempty(vec_t *v) { return !v || v->len == 0; }

/** Sets the length of 'v' to 0. */
ITER_INLINE void vec_clear(vec_t *v) {
    if (v)
        v->len = 0;
}

/** Naively sets the length of 'v'. */
ITER_INLINE void vec_setlen(vec_t *v, size_t len, size_t size) {
    if (!v || vec__mul(len, size))
        return;

    size_t newLen = len * size;
    v->len = newLen < v->cap ? newLen : v->cap;
}

/** Naively sets the capacity of 'v'. */
ITER_INLINE void vec_setcap(vec_t *v, size_t cap, size_t size) {
    if (!v || vec__mul(cap, size))
        return;

    v->cap = cap * size;

    if (v->len > v->cap)
        v->len = v->cap;
}

/** Resizes the item buffer to fit 'cap' items. */
ITER_API int vec_resize(vec_t *v, size_t cap, size_t size);

/** Reserves space for at least 'count' items. */
ITER_API int vec_reserve(vec_t *v, size_t count, size_t size);

/** Inserts 'count' items start at index 'i'. */
ITER_API int vec_insert(
    vec_t *v, void *items, size_t i, size_t count, size_t size
);

/** Inserts 'count' items at the end of the item buffer. */
ITER_API int vec_push(vec_t *v, void *items, size_t count, size_t size);

/** Inserts 'count' items at the start of the item buffer. */
ITER_INLINE int vec_unshift(vec_t *v, void *items, size_t count, size_t size) {
    return vec_insert(v, items, 0, count, size);
}

/** Inserts 'count' items starting at index 'i' by reference. */
ITER_API int vec_insertp(
    vec_t *v, void **items, size_t i, size_t count, size_t size
);

/** Inserts 'count' copies of 'item' at index 'i' */
ITER_API int vec_fill(
    vec_t *v, void *item, size_t i, size_t count, size_t size
);

/** Removes 'count' items starting at index 'i'. */
ITER_API int vec_remove(
    vec_t *v, void *out, size_t i, size_t count, size_t size
);

/** Removes 'count' items at the end of the item buffer. */
ITER_API int vec_pop(vec_t *v, void *out, size_t count, size_t size);

/** Removes 'count' items at the start of the item buffer. */
ITER_INLINE int vec_shift(vec_t *v, void *out, size_t count, size_t size) {
    return vec_remove(v, out, 0, count, size);
}

/** Swaps 'count' items starting at indices 'i' and 'j'. */
ITER_API int vec_swap(vec_t *v, size_t i, size_t j, size_t count, size_t size);

/** Removes 'count' items by swapping items. */
ITER_API int vec_swap_remove(
    vec_t *v, void *out, size_t i, size_t count, size_t size
);

/** Calls 'fn' for each item present in 'v'. */
ITER_API int vec_each(vec_t *v, vec_each_fn *fn, void *user, size_t size);

#endif
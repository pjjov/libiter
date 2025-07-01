/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_ITER_H
#define LIBITER_ITER_H

#include <iter/error.h>
#include <iter/generic.h>
#include <pf_overflow.h>
#include <stddef.h>

#ifndef ITER_API
    #define ITER_API static inline
#endif

/** ## iter(T) - Iterator interface

    `iter(T)` is an interface for traversing items of containers.
    With iterators, you can use items without knowing the details
    about their containers.
**/
#define iter(T) generic_container(iter_t, size_t, T)
typedef struct iter_t iter_t;

/** typedef int(iter_fn)(iter_t *self, void *out, size_t size, size_t skip);

    This function callback is used to implement custom iterators.

    The function should return the item, that is `size` bytes in size, after
    skipping `skip` items. If no items are found at the time of calling,
    ITER_ENODATA should be returned. The iterator can resume iteration even
    if the previous call returned ITER_ENODATA.

    When `out` and `self` are equal, the iterator should free it's resources.
**/
typedef int(iter_fn)(iter_t *iter, void *out, size_t size, size_t skip);

/** struct iter_t;

    This structure is used as an interface for traversing items of containers.
    Every member other than `call` is not used by the interface; they are
    reserved for use by the iterator's callback and should be treated as
    private variables.
**/
struct iter_t {
    iter_fn *call;
    void *container;
    const void *bucket;
    const void *current;
};

#define iter_type(m_iter) generic_value_type(iter_t, m_iter)
#define iter_type_ptr(m_iter) generic_value_ptr(iter_t, m_iter)
#define iter_type_size(m_iter) generic_value_size(iter_t, m_iter)
#define iter_as_base(m_iter) generic_check_container(iter_t, size_t, m_iter)
#define iter_check_type(m_iter, m_item)         \
    generic_check_value(iter_t, m_iter, m_item)

ITER_API int iter__call(iter_t *it, void *out, size_t size, size_t skip) {
    return it && it->call ? it->call(it, out, size, skip) : ITER_EINVAL;
}

/** int iter_next(iter(T) it, T *out)

    Advances the iterator and puts the next value in `out`, if not `NULL`.
    Possible error codes: ITER_EINVAL, ITER_ENODATA.
**/
#define iter_next(m_iter, m_out)        \
    iter__call(                         \
        iter_as_base(m_iter),           \
        iter_check_type(m_iter, m_out), \
        iter_type_size(m_iter),         \
        0                               \
    )

/** int iter_nth(iter(T) it, T *out, size_t skip)

    Skips the first `skip` items and puts the next value in `out`,
    if not `NULL`. If `skip == 0` it is equivalent to `iter_next`.
    Possible error codes: ITER_EINVAL, ITER_ENODATA.
**/
#define iter_nth(m_iter, m_out, m_n)    \
    iter__call(                         \
        iter_as_base(m_iter),           \
        iter_check_type(m_iter, m_out), \
        iter_type_size(m_iter),         \
        (m_n)                           \
    )

/** int iter_advance(iter(T) it, size_t count);

    Advances the iterator `count` times. If `count` is zero, iterator won't
    be advanced, but might be updated, depending on it's implementation.
    Possible error codes: ITER_EINVAL, ITER_ENODATA.
**/
#define iter_advance(m_iter, m_n)                                         \
    iter__call(iter_as_base(m_iter), NULL, iter_type_size(m_iter), (m_n))

/** void iter_free(iter(T) it);

    Frees all resources used by `it`, if the iterator has not been exhausted.
**/
#define iter_free(m_iter)                                                     \
    iter__call(                                                               \
        iter_as_base(m_iter), iter_as_base(m_iter), iter_type_size(m_iter), 0 \
    )

/** size_t iter_to_array(iter(T) it, T *out, size_t length);

    Traverser the iterator `it` and inserts the items into `out`, until
    `length` items have been collected or the iterator has been exhausted.
**/
#define iter_to_array(m_iter, m_out, m_length) \
    iter__to_array(                            \
        iter_as_base(m_iter),                  \
        iter_check_type(m_iter, m_out),        \
        (m_length),                            \
        iter_type_size(m_iter)                 \
    )

size_t iter__to_array(iter_t *it, void *out, size_t length, size_t stride);

/** iter(T) iter_from_array(iter_t *out, const T *items, size_t length);

    Creates an iterator that traverses the items of the provided array `items`.
**/
#define iter_from_array(m_out, m_items, m_length)           \
    ((iter(typeof(*(m_items))))iter__from_array(            \
        (m_out),                                            \
        (m_items),                                          \
        pf_checked_umulsize((m_length), sizeof(*(m_items))) \
    ))

iter_t *iter__from_array(iter_t *out, const void *items, size_t length);

#endif

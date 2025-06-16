/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_VECTOR_H
#define LIBITER_VECTOR_H

#include "error.h"
#include "generic.h"
#include <allocator.h>
#include <stddef.h>

#ifndef ITER_API
    #define ITER_API static inline
#endif

/** # vector(T) - Growable arrays

    vector(T) is a growable array like `std::vector` from C++ and `Vec`
    from Rust. The type `T` must have a size larger than 0, i.e. not `void`.

    Items are stored sequentially and, because of resizing, their memory address
    might be changed (usually by functions that can return ITER_ENOMEM). This
    means that you should never store the pointers, but rather indexes of items.

    Unlike other containers, the alignment of type `T` is handled by
    the allocator. To support over-aligned types, like SIMD vectors,
    you should use `vector_with_alignment`.
**/

#define vector(T) generic_container(vector_t, size_t, T)

typedef struct vector_t {
    void *items;
    size_t length;
    size_t capacity;
    allocator_t *allocator;
} vector_t;

/** ## Alignment

    Unlike other containers, to keep the implementation of vectors simple,
    the alignment of types is not explicitly handled and can cause problems
    for over-aligned types, like SIMD vectors.

    However, you can still use vectors in these situations if you handle
    the alignment correctly in a custom allocator that is passed to the vector,
    e.g. you can use `allocator_aligned.h` by passing a minimum alignment.
**/

#define vector_type(m_vec) generic_value_type(vector_t, m_vec)
#define vector_type_ptr(m_vec) generic_value_ptr(vector_t, m_vec)
#define vector_type_size(m_vec) generic_value_size(vector_t, m_vec)
#define vector_as_base(m_vec) generic_check_container(vector_t, size_t, m_vec)
#define vector_check_type(m_vec, m_item)         \
    generic_check_value(vector_t, m_vec, m_item)

/** vector(T) vector_init(type T, vector_t *vec, allocator_t *allocator);

    Initializes `vec` and casts it to `vector(T)`. With this function, you can
    allocate the vector object onto the stack. If `allocator` is `NULL`, the
    default one will be used instead. Returns `NULL` if unsuccessful.

    > You should use `vector_free` to free the returned vector's resources
    > instead of `vector_destroy` when this function is used.
**/
#define vector_init(T, m_vec, m_allocator)            \
    ((vector(T))vector__init((m_vec), (m_allocator)))

vector_t *vector__init(vector_t *vec, allocator_t *allocator);

/** void vector_free(vector_t *vec);

    Frees all resources used by `vec`, which was initialized
    by `vector_init` beforehand, if it's not `NULL`.
**/
#define vector_free(m_vec) vector__free((m_vec))
void vector__free(vector_t *vec);

/** vector(T) vector_create(type T, allocator_t *allocator);

    Creates a new instance of `vector(T)`, allocated with `allocator`.
    Returns `NULL` if out of memory or `sizeof(T) == 0`.
**/
#define vector_create(T, m_allocator) ((vector(T))vector__create((m_allocator)))
vector_t *vector__create(allocator_t *allocator);

/** vector(T) vector_wrap(T *array, size_t length, allocator_t *allocator);

    Creates a new instance of `vector(T)` and uses `array` as it's buffer.
    Unlike `vector_from_array`, this function takes ownership of `array`,
    managing it's lifetime with `allocator`. If `allocator` is `NULL`,
    the array will not be resized or freed.

    Returns `NULL` if out of memory or `sizeof(T) == 0`.
**/
#define vector_wrap(m_items, m_length, m_allocator)               \
    ((vector(typeof(*(m_items))))vector__wrap(                    \
        (m_items), sizeof(*(m_items)) * (m_length), (m_allocator) \
    ))

vector_t *vector__wrap(void *items, size_t length, allocator_t *allocator);

/** T *vector_unwrap(vector(T) vec);

    Destroys the `vector` without deallocating it's items, returning
    the array instead. If vector's capacity is 0, `NULL` is returned.
**/
#define vector_unwrap(m_vec)                                        \
    ((vector_type_ptr(m_vec))vector__unwrap(vector_as_base(m_vec)))

void *vector__unwrap(vector_t *vec);

/** void vector_destroy(vector(T) vec);

    Frees all resources used by `vector` if it's not `NULL`.
**/
#define vector_destroy(m_vec) vector__destroy(vector_as_base(m_vec))
void vector__destroy(vector_t *vec);

/** T *vector_slot(vector(T) vec, size_t i);

    Returns pointer to the item slot at index `i` or `NULL` if out of bounds.

    > This pointer is valid until `vector` is resized or destroyed.
**/
#define vector_slot(m_vec, m_i)                                                \
    ((vector_type_ptr(m_vec))                                                  \
         vector__slot(vector_as_base(m_vec), vector_type_size(m_vec) * (m_i)))

static inline void *vector__slot(const vector_t *vec, size_t i) {
    if (!vec || i >= vec->capacity)
        return NULL;
    return &((unsigned char *)vec->items)[i];
}

/** T *vector_get(vector(T) vec, size_t i);

    Returns pointer to the item at index `i` or `NULL` if out of bounds.

    > This pointer is valid until `vector` is resized or destroyed.
**/
#define vector_get(m_vec, m_index)                                 \
    ((vector_type_ptr(m_vec))vector__get(                          \
        vector_as_base(m_vec), vector_type_size(m_vec) * (m_index) \
    ))

ITER_API void *vector__get(const vector_t *vec, size_t i) {
    if (!vec || i >= vec->length)
        return NULL;
    return &((unsigned char *)vec->items)[i];
}

/** T *vector_items(vector(T) vec);

    Returns pointer to the internal buffer used by `vector`
    or `NULL` if `vector` is `NULL` or it's capacity is zero.

    > This pointer is valid until `vector` is resized or destroyed.
**/
#define vector_items(m_vec)                                        \
    (vector_type_ptr(m_vec))(vector__items(vector_as_base(m_vec)))

ITER_API void *vector__items(const vector_t *vec) {
    return vec ? vec->items : NULL;
}

/** T *vector_end(vector(T) vec);

    Returns pointer to the end of the buffer used by `vector`.
    or `NULL` if `vector` is `NULL` or it's capacity is zero.

    > This pointer is valid until `vector` is resized or destroyed.
**/
#define vector_end(m_vec)                                        \
    (vector_type_ptr(m_vec))(vector__end(vector_as_base(m_vec)))

ITER_API void *vector__end(const vector_t *vec) {
    return vec ? &((unsigned char *)vec->items)[vec->length] : NULL;
}

/** size_t vector_length(vector(T) vec);

    Returns number of items present in `vec` or 0 if `vec` is `NULL`.
**/
#define vector_length(m_vec)                                          \
    (vector__length(vector_as_base(m_vec)) / vector_type_size(m_vec))

ITER_API size_t vector__length(const vector_t *vec) {
    return vec ? vec->length : 0;
}

/** size_t vector_capacity(vector(T) vec);

    Returns number of items for which space is reserved in `vec`.
**/
#define vector_capacity(m_vec)                                          \
    (vector__capacity(vector_as_base(m_vec)) / vector_type_size(m_vec))

ITER_API size_t vector__capacity(const vector_t *vec) {
    return vec ? vec->capacity : 0;
}

/** size_t vector_bytes_used(vector(T) vec);

    Returns number of bytes used by items present in `vector`.
**/
#define vector_bytes_used(m_vec) (vector__length(vector_as_base(m_vec)))

/** size_t vector_bytes_reserved(vector(T) vec);

    Returns number of bytes reserved for items in `vector`.
**/
#define vector_bytes_reserved(m_vec) (vector__capacity(vector_as_base(m_vec)))

/** allocator_t *vector_allocator(vector(T) vec);

    Returns the allocator used by `vec` or `NULL`
    if `vec` was created with `vector_wrap`.
**/
#define vector_allocator(m_vec) (vector__allocator(vector_as_base(m_vec)))

ITER_API allocator_t *vector__allocator(const vector_t *vec) {
    return vec ? vec->allocator : NULL;
}

/** int vector_is_empty(vector(T) vec);

    Returns a non-zero value if `vec` has no items.
**/
#define vector_is_empty(m_vec) (vector_length(m_vec) == 0)

/** int vector_resize(vector(T) vec, size_t capacity);

    Resizes vector's buffer to fit `capacity` items. If the length of
    `vec` is larger than `capacity`, excess items will be removed.

    > It's better to use `vector_reserve` for reserving space.

    Possible error codes: `ITER_EINVAL`, `ITER_ENOMEM`.
**/
#define vector_resize(m_vec, m_capacity)                              \
    vector__resize(                                                   \
        vector_as_base(m_vec), vector_type_size(m_vec) * (m_capacity) \
    )

int vector__resize(vector_t *vec, size_t capacity);

/** int vector_reserve(vector(T) vec, size_t count);

    Reserves space to fit at least `count` more items in `vec`.
    This function will not resize the buffer if space is already reserved.

    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_reserve(m_vec, m_count) \
    vector__reserve(vector_as_base(m_vec), vector_type_size(m_vec) * (m_count))

int vector__reserve(vector_t *vec, size_t size);

/** int vector_shrink(vector(T) vec);

    Resizes internal buffer to fit least space possible.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_shrink(m_vec) vector__shrink(vector_as_base(m_vec))

ITER_API int vector__shrink(vector_t *vec) {
    return vector__resize(vec, vector__length(vec));
}

/** int vector_insert(vector(T) vec, T *items, size_t i, size_t count);

    Inserts `count` items from `items` starting at index `i`.
    `count` must not be zero and `i` must not be larger than length.

    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_insert(m_vec, m_items, m_i, m_count) \
    vector__insert(                                 \
        vector_as_base(m_vec),                      \
        vector_check_type(m_vec, m_items),          \
        vector_type_size(m_vec) * (m_i),            \
        vector_type_size(m_vec) * (m_count)         \
    )

int vector__insert(vector_t *vec, const void *items, size_t i, size_t size);

/** int vector_push(vector(T) vec, T *items, size_t count);

    Inserts `count` items from `items`, starting from the end of `vector`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_push(m_vec, m_items, m_count) \
    vector__insert(                          \
        vector_as_base(m_vec),               \
        vector_check_type(m_vec, m_items),   \
        vector_type_size(m_vec) * (m_count)  \
    )

ITER_API int vector__push(vector_t *vec, const void *items, size_t size) {
    return vector__insert(vec, items, vector__length(vec), size);
}

/** int vector_try_insert(vector(T) vec, T *items, size_t i, size_t count);

    Attempts to insert `count` items from `items` starting at index `i` without
    reserving additional space, i.e. without resizing if out of memory.

    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_try_insert(m_vec, m_items, m_i, m_count) \
    vector__try_insert(                                 \
        vector_as_base(m_vec),                          \
        vector_check_type(m_vec, m_items),              \
        vector_type_size(m_vec) * (m_i),                \
        vector_type_size(m_vec) * (m_count)             \
    )

ITER_API int vector__try_insert(
    vector_t *vec, const void *items, size_t i, size_t size
) {
    if (vec && vec->length + size > vec->capacity)
        return ITER_ENOMEM;
    return vector__insert(vec, items, i, size);
}

#endif

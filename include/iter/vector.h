/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_VECTOR_H
#define LIBITER_VECTOR_H

#include <iter/error.h>
#include <iter/generic.h>

typedef struct allocator_t allocator_t;
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
#define vector_type_mul(m_vec, m_i)                                    \
    vector__checked_umulsize(generic_value_size(vector_t, m_vec), m_i)

#define vector_as_base(m_vec) generic_check_container(vector_t, size_t, m_vec)
#define vector_check_type(m_vec, m_item)         \
    generic_check_value(vector_t, m_vec, m_item)

#ifndef VECTOR_ALLOW_OVERFLOW
    #define PF_OVERFLOW_SKIP_DEFAULT
    #include <pf_overflow.h>
    #undef PF_OVERFLOW_SKIP_DEFAULT

PF_IMPL_OVERFLOW(PF_OVERFLOW_SIZE, size_t, u, size, vector_)
#else
    #define vector__checked_umulsize(m_l, m_r) ((m_l) * (m_r))
#endif

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

/** vector(T) vector_with_capacity(type T, size_t cap, allocator_t *allocator);

    Creates a new instance of `vector(T)`, allocated with `allocator`, and
    reserves space to fit least `cap` items. Returns `NULL` if out of memory.

    > If `allocator` is `NULL`, the default one will be used.
**/
#define vector_with_capacity(T, m_capacity, m_allocator)                 \
    ((vector(T))vector__with_capacity(                                   \
        vector__checked_umulsize(sizeof(T), (m_capacity)), (m_allocator) \
    ))

vector_t *vector__with_capacity(size_t cap, allocator_t *allocator);

/** vector(T) vector_from_array(T *array, size_t len, allocator_t *allocator);

    Creates a new vector with type `T` and copies `len` items from `array`.
    Unlike `vector_wrap`, this function copies `array` into a new buffer.
    Returns `NULL` if out of memory or `sizeof(T) == 0`.
**/
#define vector_from_array(m_items, m_length, m_allocator)         \
    ((vector(typeof(*(m_items))))vector__from_array(              \
        (const void *)(m_items),                                  \
        vector__checked_umulsize(sizeof(*(m_items)), (m_length)), \
        (m_allocator)                                             \
    ))

vector_t *vector__from_array(
    const void *items, size_t length, allocator_t *alloc
);

/** vector(T) vector_from_iter(iter(T) it, allocator_t *allocator)

    Creates a new vector with type `T` and inserts all items from `it`.
    Returns `NULL` if out of memory or `sizeof(T) == 0`.
**/
#define vector_from_iter(m_it, m_allocator)                     \
    ((vector(iter_type(m_it)))vector__from_iter(                \
        iter_as_base(m_it), (m_allocator), iter_type_size(m_it) \
    ))

vector_t *vector__from_iter(iter_t *it, allocator_t *allocator, size_t stride);

/** vector(T) vector_wrap(T *array, size_t length, allocator_t *allocator);

    Creates a new instance of `vector(T)` and uses `array` as it's buffer.
    Unlike `vector_from_array`, this function takes ownership of `array`,
    managing it's lifetime with `allocator`. If `allocator` is `NULL`,
    the array will not be resized or freed.

    Returns `NULL` if out of memory or `sizeof(T) == 0`.
**/
#define vector_wrap(m_items, m_length, m_allocator)               \
    ((vector(typeof(*(m_items))))vector__wrap(                    \
        (m_items),                                                \
        vector__checked_umulsize(sizeof(*(m_items)), (m_length)), \
        (m_allocator)                                             \
    ))

vector_t *vector__wrap(void *items, size_t length, allocator_t *allocator);

/** vector(T) vector_clone(vector(T) vec, allocator_t *allocator);

    Creates a new `vector(T)`, using `allocator`, and copies all items
    from `vec`. Returns `NULL` if out of memory or if `vec` is empty.
**/
#define vector_clone(m_vec, m_allocator)                                 \
    ((typeof(m_vec))vector__clone(vector_as_base(m_vec), (m_allocator)))

ITER_API vector_t *vector__clone(const vector_t *vec, allocator_t *allocator) {
    return vec ? vector__from_array(vec->items, vec->length, allocator) : NULL;
}

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
#define vector_slot(m_vec, m_i)                                            \
    ((vector_type_ptr(m_vec))                                              \
         vector__slot(vector_as_base(m_vec), vector_type_mul(m_vec, m_i)))

static inline void *vector__slot(const vector_t *vec, size_t i) {
    if (!vec || i >= vec->capacity)
        return NULL;
    return &((unsigned char *)vec->items)[i];
}

/** T *vector_get(vector(T) vec, size_t i);

    Returns pointer to the item at index `i` or `NULL` if out of bounds.

    > This pointer is valid until `vector` is resized or destroyed.
**/
#define vector_get(m_vec, m_index)                                            \
    ((vector_type_ptr(m_vec))                                                 \
         vector__get(vector_as_base(m_vec), vector_type_mul(m_vec, m_index)))

ITER_API void *vector__get(const vector_t *vec, size_t i) {
    if (!vec || i >= vec->length)
        return NULL;
    return &((unsigned char *)vec->items)[i];
}

/** size_t vector_index(vector(T) vec, T *item);

    Returns the index of the item pointed at by `item`.
    Possible error codes: ITER_EINVAL.
**/
#define vector_index(m_vec, m_item)                                         \
    (vector__index(vector_as_base(m_vec), vector_check_type(m_vec, m_item)) \
     / vector_type_size(m_vec))

size_t vector__index(const vector_t *vec, const void *item);

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

/** void vector_clear(vector(T) vec);

    Removes all items from `vec`, if it's not `NULL`.
**/
#define vector_clear(m_vec) vector__clear(vector_as_base(m_vec))

ITER_API void vector__clear(vector_t *vec) {
    if (vec)
        vec->length = 0;
}

/** int vector_set_length(vector(T) vec, size_t length);

    Forces the length of `vec` to the value of `length`.
    `length` must be less than or equal to `vec`'s capacity.
    Possible error codes: `ITER_EINVAL`.
**/
#define vector_set_length(m_vec, m_length) \
    vector__set_length(vector_as_base(m_vec), vector_type_mul(m_vec, m_length))

ITER_API size_t vector__set_length(vector_t *vec, size_t length) {
    if (!vec || length > vec->capacity)
        return ITER_EINVAL;

    vec->length = length;
    return ITER_OK;
}

/** int vector_resize(vector(T) vec, size_t capacity);

    Resizes vector's buffer to fit `capacity` items. If the length of
    `vec` is larger than `capacity`, excess items will be removed.

    > It's better to use `vector_reserve` for reserving space.

    Possible error codes: `ITER_EINVAL`, `ITER_ENOMEM`.
**/
#define vector_resize(m_vec, m_capacity)                                      \
    vector__resize(vector_as_base(m_vec), vector_type_mul(m_vec, m_capacity))

int vector__resize(vector_t *vec, size_t capacity);

/** int vector_reserve(vector(T) vec, size_t count);

    Reserves space to fit at least `count` more items in `vec`.
    This function will not resize the buffer if space is already reserved.

    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_reserve(m_vec, m_count)                                      \
    vector__reserve(vector_as_base(m_vec), vector_type_mul(m_vec, m_count))

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
        vector_type_mul(m_vec, m_i),                \
        vector_type_mul(m_vec, m_count)             \
    )

int vector__insert(vector_t *vec, const void *items, size_t i, size_t size);

/** int vector_push(vector(T) vec, T *items, size_t count);

    Inserts `count` items from `items`, starting from the end of `vector`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_push(m_vec, m_items, m_count) \
    vector__push(                            \
        vector_as_base(m_vec),               \
        vector_check_type(m_vec, m_items),   \
        vector_type_mul(m_vec, m_count)      \
    )

ITER_API int vector__push(vector_t *vec, const void *items, size_t size) {
    return vector__insert(vec, items, vector__length(vec), size);
}

/** int vector_insert_at(vector_t *vec, T *items, T *at, size_t size);

    Unlike `vector_insert`, this function inserts at the item pointed by `at`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_insert_at(m_vec, m_items, m_at, m_count) \
    vector__insert_at(                                  \
        vector_as_base(m_vec),                          \
        vector_check_type(m_vec, m_items),              \
        vector_check_type(m_vec, m_at),                 \
        vector_type_mul(m_vec, (m_count))               \
    )

ITER_API int vector__insert_at(
    vector_t *vec, const void *items, void *at, size_t size
) {
    size_t i = vector__index(vec, at);
    return at ? vector__insert(vec, items, i, size) : ITER_EINVAL;
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
        vector_type_mul(m_vec, m_i),                    \
        vector_type_mul(m_vec, m_count)                 \
    )

ITER_API int vector__try_insert(
    vector_t *vec, const void *items, size_t i, size_t size
) {
    if (vec && vec->length + size > vec->capacity)
        return ITER_ENOMEM;
    return vector__insert(vec, items, i, size);
}

/** int vector_remove(vector(T) vec, size_t i, size_t count);

    Removes `count` items from `vec` starting from the index `i`.
    Possible error codes: ITER_EINVAL.
**/
#define vector_remove(m_vec, m_i, m_count) \
    vector__remove(                        \
        vector_as_base(m_vec),             \
        vector_type_mul(m_vec, (m_i)),     \
        vector_type_mul(m_vec, (m_count))  \
    )

int vector__remove(vector_t *vec, size_t i, size_t size);

/** int vector_pop(vector(T) vec, size_t count);

    Removes `count` items from the end of the `vec`.
    Possible error codes: ITER_EINVAL.
**/
#define vector_pop(m_vec, m_count)                                        \
    vector__pop(vector_as_base(m_vec), vector_type_mul(m_vec, (m_count)))

ITER_API int vector__pop(vector_t *vec, size_t count) {
    if (!vec || count > vec->length)
        return ITER_EINVAL;
    return vector__remove(vec, vec->length - count, count);
}

/** int vector_remove_at(vector(T) vec, T *at, size_t count);

    Removes `count` items at the location pointed by `at`.
    Possible error codes: ITER_EINVAL.
**/
#define vector_remove_at(m_vec, m_at, m_count) \
    vector__remove_at(                         \
        vector_as_base(m_vec),                 \
        vector_check_type(m_vec, m_at),        \
        vector_type_mul(m_vec, (m_count))      \
    )

ITER_API int vector__remove_at(vector_t *vec, void *at, size_t size) {
    return vector__remove(vec, vector__index(vec, at), size);
}

/** int vector_swap_remove(vector(T) vec, size_t i, size_t count);

    Removes `count` items starting at index `i` by moving last `count` items
    in their place. Possible error codes: ITER_EINVAL.

    > While faster than `remove`, this function doesn't preserve item order.
**/
#define vector_swap_remove(m_vec, m_i, m_count) \
    vector__swap_remove(                        \
        vector_as_base(m_vec),                  \
        vector_type_mul(m_vec, (m_i)),          \
        vector_type_mul(m_vec, (m_count))       \
    )

int vector__swap_remove(vector_t *vec, size_t i, size_t size);

/** int vector_swap(vector(T) vec, size_t i, size_t j, size_t count);

    Swaps `count` items starting at index `i` with items at index `j`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define vector_swap(m_vec, m_i, m_j, m_count) \
    vector__swap(                             \
        vector_as_base(m_vec),                \
        vector_type_mul(m_vec, (m_i)),        \
        vector_type_mul(m_vec, (m_j)),        \
        vector_type_mul(m_vec, (m_count))     \
    )

int vector__swap(vector_t *vec, size_t i, size_t j, size_t size);

/** int vector_each(vector(T) vec, vector_each_fn *each, void *user);

    Calls the `each` callback for each item present in `vec`,
    stopping if a non-zero value is returned by one of the calls.

    ```c
    typedef int(vector_each_fn)(void *item, void *user);
    ```

    Possible error codes: ITER_EINVAL, ITER_EINTR.
**/
#define vector_each(m_vec, m_each, m_user)                                 \
    vector__each(                                                          \
        vector_as_base(m_vec), (m_each), (m_user), vector_type_size(m_vec) \
    )

typedef int(vector_each_fn)(void *item, void *user);
int vector__each(vector_t *vec, vector_each_fn *each, void *user, size_t size);

/** int vector_filter(vector(T) vec, vector_each_fn *filter, void *user);

    Removes each item in `vec` for which the `filter` callback returns a `0`.
    Possible error codes: ITER_EINVAL.
**/
#define vector_filter(m_vec, m_filter, m_user)                               \
    vector__filter(                                                          \
        vector_as_base(m_vec), (m_filter), (m_user), vector_type_size(m_vec) \
    )

int vector__filter(
    vector_t *vec, vector_each_fn *filter, void *user, size_t size
);

/** int vector_map(vector(D) dst, vector(S), src, vector_map_fn *map, void *user);

    Maps items of type `S` to items of type `D` using the `map` callback,
    stopping if a non-zero value is returned by one of the calls.

    ```c
    typedef int(vector_map_fn)(void *dst, void *src, void *user);
    ```

    Possible error codes: ITER_EINVAL, ITER_EINTR, ITER_ENOMEM.
**/
#define vector_map(m_dst, m_src, m_map, m_user) \
    vector__map(                                \
        vector_as_base(m_dst),                  \
        vector_as_base(m_src),                  \
        (m_map),                                \
        (m_user),                               \
        vector_type_size(m_dst),                \
        vector_type_size(m_src)                 \
    )

typedef int(vector_map_fn)(void *dst, void *src, void *user);

int vector__map(
    vector_t *dst,
    vector_t *src,
    vector_map_fn *map,
    void *user,
    size_t dsize,
    size_t ssize
);

/** T *vector_find(vector(T) vec, const T *item, vector_compare_fn *cmp);

    Finds `item` inside `vec` by comparing items with `cmp`.
    If the `vector_compare_fn` callback is `NULL` it will default to `memcmp`.

    ```c
    typedef int(vector_compare_fn)(const void *lhs, const void *rhs, size_t s);
    ```
**/
#define vector_find(m_vec, m_item, m_cmp) \
    vector__find(                         \
        vector_as_base(m_vec),            \
        vector_check_type(m_vec, m_item), \
        vector_type_size(m_vec),          \
        (m_cmp)                           \
    )

typedef int(vector_compare_fn)(const void *lhs, const void *rhs, size_t size);

void *vector__find(
    vector_t *vec, const void *item, size_t size, vector_compare_fn *cmp
);

/** iter(T) vector_iter(vector(T) vec, iter_t *out);

    Initializes `out` as an iterator traversing items present in `vec`.

    > Resizing the vector will invalidate the returned iterator.
**/
#define vector_iter(m_vec, m_out)                                            \
    ((iter(vector_type(m_vec)))vector__iter(vector_as_base(m_vec), (m_out)))

iter_t *vector__iter(vector_t *vec, iter_t *out);

#endif

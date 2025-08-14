#ifndef ITER_POOL_H
#define ITER_POOL_H

typedef struct allocator_t allocator_t;
#include <iter/generic.h>
#include <iter/iter.h>

#ifndef ITER_API
    #define ITER_API static inline
#endif

/** ## pool(T)

    Iterable container with fast insertion and deletion operations.
    The item pointers returned by this structure will be valid
    until the item has been destroyed by using `pool_give`.
**/
#define pool(T) generic_container(pool_t, size_t, T)

typedef struct pool_t {
    void *buffer;
    size_t size;
    size_t align;
    size_t count;
    size_t capacity;
    allocator_t *allocator;
} pool_t;

#define pool_type(m_pool) generic_value_type(pool_t, m_pool)
#define pool_type_ptr(m_pool) generic_value_ptr(pool_t, m_pool)
#define pool_type_size(m_pool) generic_value_size(pool_t, m_pool)
#define pool_as_base(m_pool) generic_check_container(pool_t, size_t, m_pool)
#define pool_check_type(m_pool, m_item)         \
    generic_check_value(pool_t, m_pool, m_item)

/** pool(T) pool_init(type T, pool_t *out, allocator_t *allocator);

    Initializes `out` and casts it to `pool(T)`. With this function, you can
    allocate the pool object onto the stack. If `allocator` is `NULL`, the
    default one will be used instead. Returns `NULL` if unsuccessful.

    > You should use `pool_free` to free the returned pool's resources
    > instead of `pool_destroy` when this function is used.
**/
#define pool_init(T, m_out, m_allocator)                                 \
    ((pool(T))pool__init((m_out), (m_allocator), sizeof(T), alignof(T)))

pool_t *pool__init(pool_t *out, allocator_t *alloc, size_t size, size_t align);

/** void pool_free(pool_t *pool);

    Frees all resources used by `pool`, which was initialized
    by `pool_init` beforehand, if it's not `NULL`.
**/
#define pool_free(m_pool) pool__free((m_pool))

void pool__free(pool_t *pool);

/** pool(T) pool_create(type T, allocator_t *allocator);

    Creates a new instance of `pool(T)`, allocated with `allocator`.
    Returns `NULL` if out of memory or `sizeof(T) == 0`.

    > If `allocator` is `NULL`, the default one will be used.
**/
#define pool_create(T, m_allocator)                               \
    ((pool(T))pool__create((m_allocator), sizeof(T), alignof(T)))

pool_t *pool__create(allocator_t *alloc, size_t size, size_t align);

/** pool(T) pool_with_capacity(type T, size_t cap, allocator_t *allocator);

    Creates a new instance of `pool(T)`, allocated with `allocator`, and
    reserves space to fit least `cap` items. Returns `NULL` if out of
    memory or `sizeof(T) == 0`.

    > If `allocator` is `NULL`, the default one will be used.
**/
#define pool_with_capacity(T, m_capacity, m_allocator)     \
    ((pool(T))pool__with_capacity(                         \
        (m_capacity), (m_allocator), sizeof(T), alignof(T) \
    ))

pool_t *pool__with_capacity(
    size_t capacity, allocator_t *allocator, size_t size, size_t alignment
);

/** void pool_destroy(pool(T) pool);

    Frees all resources used by `pool` if it's not `NULL`.
**/
#define pool_destroy(m_pool) pool__destroy(pool_as_base(m_pool))

void pool__destroy(pool_t *pool);

/** int pool_reserve(pool(T) pool, size_t count);

    Reserves space to fit at least `count` more items in `pool`.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define pool_reserve(m_pool, m_count)              \
    pool__reserve(pool_as_base(m_pool), (m_count))

int pool__reserve(pool_t *pool, size_t count);

/** size_t pool_capacity(pool(T) pool);

    Returns number of items for which space is
    reserved in `pool` or 0 if `pool` is `NULL`.
**/
#define pool_capacity(m_pool) (pool__capacity(pool_as_base(m_pool)))

ITER_API size_t pool__capacity(pool_t *pool) {
    return pool ? pool->capacity : 0;
}

/** size_t pool_count(pool(T) pool);
    Returns number of items present in `pool` or 0 if `pool` is `NULL`.
**/
#define pool_count(m_pool) (pool__count(pool_as_base(m_pool)))

ITER_API size_t pool__count(pool_t *pool) { return pool ? pool->count : 0; }

/** allocator_t *pool_allocator(pool(T) pool);

    Returns the allocator used by `pool`.
**/
#define pool_allocator(m_pool) (pool__allocator(pool_as_base(m_pool)))

ITER_API allocator_t *pool__allocator(pool_t *pool) {
    return pool ? pool->allocator : NULL;
}

/** T *pool_take(pool(T) pool)

    Takes an unused item from the pool and returns it.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define pool_take(m_pool)                                     \
    ((pool_type_ptr(m_pool))pool__take(pool_as_base(m_pool)))

void *pool__take(pool_t *pool);

/** int pool_give(pool(T) pool, T *item)

    Gives back the item to the `pool`, marking it as unused.
    Possible error codes: ITER_EINVAL, ITER_ENOENT.
**/
#define pool_give(m_pool, m_item)                                     \
    pool__give(pool_as_base(m_pool), pool_check_type(m_pool, m_item))

int pool__give(pool_t *pool, void *item);

/** iter(T) pool_iter(pool(T) pool);

    Creates a iterator that returns items taken from the pool.
    > The returned iterator cannot be used after destroying it's pool.
**/
#define pool_iter(m_pool)                                       \
    ((iter(pool_type(m_pool)))pool__iter(pool_as_base(m_pool)))

iter_t *pool__iter(pool_t *pool);

#endif

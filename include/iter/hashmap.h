/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_HASHMAP_H
#define LIBITER_HASHMAP_H

#include <allocator.h>
#include <iter/generic.h>
#include <iter/hash.h>
#include <iter/iter.h>

#ifndef ITER_API
    #define ITER_API static inline
#endif

/** ## hashmap(K, V) - Associative arrays

    Hash maps are associative containers, storing key-value pairs.
    Unlike `vector`, `hashmap` stores two types, one for the key and
    one for the value (labeled as K and V).

    The performance of this container heavily depends on the `hasher_fn` used.
**/
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

/** ## Casting

    Because of stricter memory requirements, casting the key and value types
    of hashmaps should only be done when the new types have the same size and
    alignment as the previous ones, i.e. produce the same layout structure.
**/
#define hashmap_make_layout(K, V)                                              \
    ((struct hashmap_layout) { sizeof(K), alignof(K), sizeof(V), alignof(V) })

struct hashmap_layout {
    size_t ksize;
    size_t kalign;
    size_t vsize;
    size_t valign;
};

#define hashmap_value(m_hashmap) generic_value_type(hashmap_t, m_hashmap)
#define hashmap_value_ptr(m_hashmap) generic_value_ptr(hashmap_t, m_hashmap)
#define hashmap_value_size(m_hashmap) generic_value_size(hashmap_t, m_hashmap)
#define hashmap_as_base(m_hashmap) ((hashmap_t *)(m_hashmap))
#define hashmap_check_value(m_hashmap, m_value)        \
    generic_check_value(hashmap_t, m_hashmap, m_value)
#define hashmap_check_key(m_hashmap, m_key)        \
    generic_check_key(hashmap_t, m_key, m_hashmap)

/** hashmap(K, V) hashmap_init(
        type K, type V,
        hashmap_t *map,
        allocator_t *allocator
    );

    Initializes `map` and casts it to `hashmap(K, V)`. With this function,
    you can allocate the hashmap object onto the stack. If `allocator` is `NULL`,
    the default one will be used instead. Returns `NULL` if unsuccessful.

    > You should use `hashmap_free` to free the returned hashmap's resources
    > instead of `hashmap_destroy` when this function is used.
**/
#define hashmap_init(K, V, m_out, m_allocator)                              \
    ((hashmap(K, V))                                                        \
         hashmap__init((m_out), (m_allocator), &hashmap_make_layout(K, V)))

hashmap_t *hashmap__init(
    hashmap_t *out, allocator_t *allocator, const struct hashmap_layout *layout
);

/** void hashmap_free(hashmap_t *map);

    Frees all resources used by `map`, which was initialized
    by `hashmap_init` beforehand, if it's not `NULL`.
**/
#define hashmap_free(m_map) hashmap__free(hashmap_as_base(m_map))
void hashmap__free(hashmap_t *map);

/** hashmap(K, V) hashmap_create(type K, type V, allocator_t *allocator);

    Creates a new instance of `hashmap(K, V)`, allocated with `allocator`.
    Returns `NULL` if out of memory or `sizeof(K) == 0`.

    > If `allocator` is `NULL`, the default one will be used.
**/
#define hashmap_create(K, V, m_allocator) \
    ((hashmap(K, V))hashmap__create((m_allocator), &hashmap_make_layout(K, V)))

hashmap_t *hashmap__create(
    allocator_t *allocator, const struct hashmap_layout *layout
);

/** hashmap(K, V) hashmap_with_capacity(
        type K, type V,
        size_t capacity,
        allocator_t *allocator
    );

    Creates a new instance of `hashmap(K, V)`, allocated with `allocator`,
    and reserves space for at least `capacity` more items.
    Returns `NULL` if out of memory or `sizeof(K) == 0`.

    > If `allocator` is `NULL`, the default one will be used.
**/
#define hashmap_with_capacity(K, V, m_capacity, m_allocator)    \
    ((hashmap(K, V))hashmap__with_capacity(                     \
        (m_capacity), (m_allocator), &hashmap_make_layout(K, V) \
    ))

hashmap_t *hashmap__with_capacity(
    size_t capacity, allocator_t *allocator, const struct hashmap_layout *layout
);

/** void hashmap_destroy(hashmap(K, V) map);

    Frees all resources used by `map`.
    > If `map` is `NULL`, the function silently returns.
**/
#define hashmap_destroy(m_map) hashmap__destroy(hashmap_as_base(m_map))
void hashmap__destroy(hashmap_t *map);

/** int hashmap_use_hash(hashmap(K, V) map, hash_fn *hash, hasher_fn *hasher);

    Uses the `hash` and `hasher` for storing key-value pairs.
    This function cannot be used if items are already present in `map`.
    Possible error codes: ITER_EINVAL.
**/
#define hashmap_use_hash(m_map, m_hash, m_hasher)                   \
    hashmap__use_hash(hashmap_as_base(m_map), (m_hash), (m_hasher))

int hashmap__use_hash(hashmap_t *map, hash_fn *hash, hasher_fn *hasher);

/** int hashmap_reserve(hashmap(K, V) map, size_t count);

    Reserves space to fit at least `count` more items.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define hashmap_reserve(m_map, m_count)                 \
    hashmap__reserve(hashmap_as_base(m_map), (m_count))

int hashmap__reserve(hashmap_t *map, size_t count);

/** size_t hashmap_count(const hashmap(K, V) map);

    Returns the number of items in `map`.
**/
#define hashmap_count(m_map) hashmap__count(hashmap_as_base(m_map))

ITER_API size_t hashmap__count(const hashmap_t *map) {
    return map ? map->count : 0;
}

/** size_t hashmap_capacity(const hashmap(K, V) map);

    Returns the number of slots reserved for storing items in `map`.
**/
#define hashmap_capacity(m_map) hashmap__capacity(hashmap_as_base(m_map))

ITER_API size_t hashmap__capacity(const hashmap_t *map) {
    return map && map->buffer ? 1 << map->capacityLog2 : 0;
}

/** allocator_t *hashmap_allocator(const hashmap(K, V) map);

    Returns the allocator used by `map`.
**/
#define hashmap_allocator(m_map) hashmap__allocator(hashmap_as_base(m_map))

ITER_API allocator_t *hashmap__allocator(const hashmap_t *map) {
    return map ? map->allocator : NULL;
}

/** V *hashmap_get(const hashmap(K, V) map, const K *key);

    Returns the value associated with `key`, or `NULL` if not found.
**/
#define hashmap_get(m_map, m_key)                               \
    ((hashmap_value_ptr(m_map))hashmap__get(                    \
        hashmap_as_base(m_map), hashmap_check_key(m_map, m_key) \
    ))

void *hashmap__get(const hashmap_t *map, const void *key);

/** int hashmap_set(hashmap(K, V) map, const K *key, const V *value);

    Sets the value associated with `key` to `value`, inserting if not present.
    Possible error codes: ITER_EEXIST, ITER_EINVAL, ITER_ENOMEM.
**/
#define hashmap_set(m_map, m_key, m_value)          \
    hashmap__set(                                   \
        hashmap_as_base(m_map),                     \
        (void *)hashmap_check_key(m_map, m_key),    \
        (void *)hashmap_check_value(m_map, m_value) \
    )

int hashmap__set(hashmap_t *map, const void *key, const void *value);

/** int hashmap_insert(hashmap(K, V) map, const K *key, const V *value);

    Attempts to insert the key-value pair if not already present.
    Possible error codes: ITER_EEXIST, ITER_EINVAL, ITER_ENOMEM.
**/
#define hashmap_insert(m_map, m_key, m_value)       \
    hashmap__insert(                                \
        hashmap_as_base(m_map),                     \
        (void *)hashmap_check_key(m_map, m_key),    \
        (void *)hashmap_check_value(m_map, m_value) \
    )

int hashmap__insert(hashmap_t *map, const void *key, const void *value);

/** int hashmap_remove(hashmap(K, V) map, const K *key);

    Removes the key-value pair matched by `key`, if found.
    Possible error codes: ITER_EINVAL, ITER_ENOENT.
**/
#define hashmap_remove(m_map, m_key)                                         \
    hashmap__remove(hashmap_as_base(m_map), hashmap_check_key(m_map, m_key))

int hashmap__remove(hashmap_t *map, const void *key);

/** void hashmap_clear(hashmap(K, V) map);

    Removes all items from `map`, silently returning if it's `NULL`.
**/
#define hashmap_clear(m_map) hashmap__clear(hashmap_as_base(m_map))

void hashmap__clear(hashmap_t *map);

/** int hashmap_fast_insert(hashmap(K, V) map, const K *key, const V *value);

    Attempts to insert the key-value pair without checking if they have already
    been inserted. This function can break `map` if not used carefully.
    Possible error codes: ITER_EINVAL, ITER_ENOMEM.
**/
#define hashmap_fast_insert(m_map, m_key, m_value)  \
    hashmap__fast_insert(                           \
        hashmap_as_base(m_map),                     \
        (void *)hashmap_check_key(m_map, m_key),    \
        (void *)hashmap_check_value(m_map, m_value) \
    )

int hashmap__fast_insert(hashmap_t *map, const void *key, const void *value);

/** int hashmap_each(hashmap(T) map, hashmap_each_fn *each, void *user);

    Calls the `each` callback for each item present in `map`,
    stopping if a non-zero value is returned by one of the calls.

    ```c
    typedef int(hashmap_each_fn)(void *key, void *value, void *user);
    ```

    Possible error codes: ITER_EINVAL, ITER_EINTR.
**/
#define hashmap_each(m_map, m_each, m_user)                   \
    hashmap__each(hashmap_as_base(m_map), (m_each), (m_user))

typedef int(hashmap_each_fn)(void *key, void *value, void *user);
int hashmap__each(hashmap_t *map, hashmap_each_fn *each, void *user);

/** int hashmap_filter(hashmap(T) map, hashmap_each_fn *filter, void *user);

    Removes each item in `map` for which the `filter` callback returns a `0`.
    Possible error codes: ITER_EINVAL.
**/
#define hashmap_filter(m_map, m_filter, m_user)                   \
    hashmap__filter(hashmap_as_base(m_map), (m_filter), (m_user))

int hashmap__filter(hashmap_t *map, hashmap_each_fn *filter, void *user);

/** iter(V) hashmap_iter(hashmap(K, V) map, iter_t *out);

    Initializes `out` as an iterator traversing values present in `map`.

    > Inserting items or reserving space will invalidate the returned iterator.
**/
#define hashmap_iter(m_map, m_out) \
    ((iter(hashmap_value(m_map)))hashmap__iter(hashmap_as_base(m_map), (m_out)))

iter_t *hashmap__iter(hashmap_t *map, iter_t *out);

#endif

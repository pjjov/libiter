/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_MAP_H
#define LIBITER_MAP_H

#define LIBITER_NEED_TYPE
#define LIBITER_NEED_MAP
#include <iter/types.h>

#ifndef ITER_API
    #define ITER_API
#endif

#ifndef ITER_INLINE
    #define ITER_INLINE static inline
#endif

/** Group: Helper macros */

/** Extracts the item type for a given map. */
#define map_type(MAP) generic_item_type(map_t, MAP)

/** Extracts the item type as a pointer for a given map. */
#define map_type_ptr(MAP) generic_item_ptr(map_t, MAP)

/** Extracts the size of the item type for a given map. */
#define map_type_size(MAP) generic_item_size(map_t, MAP)

/** Cast the map into `map_t *` with some type safety. */
#define map_base(MAP) generic_check_container(map_t, MAP)

/** Checks if given item has the map's subtype. */
#define map_check(MAP, ITEM) generic_check_item(map_t, MAP, ITEM)

/** Checks if given item has the map's subtype as double pointer. */
#define map_checkd(MAP, ITEM) generic_check_item_d(map_t, MAP, ITEM)

/** Group: Allocation functions
    Custom-prototype: true
*/

/** map(T) mapnew(type T, allocator_t *allocator);

    Creates a new instance of `map(T)`, allocated with 'allocator'.
    Returns: `NULL` if out of memory.
*/
#define mapnew(T, hash, allocator)                                \
    ((map(T))map_new((hash), (allocator), sizeof(T), alignof(T)))

/** map(T) mapinit(type T, map_t *out, allocator_t *allocator);

    Initializes 'out' to a new instance of `map(T)`, allocated with 'allocator'.
*/
#define mapinit(T, out, hash, allocator)                                  \
    ((map(T))map_init((out), (hash), (allocator), sizeof(T), alignof(T)))

/** void mapfree(map(T) m);

    Frees the item buffer of 'm' and frees 'm' if `mapinit` was not used.
*/
#define mapfree(map) map_free(map_base(map))

/** int mapreserve(map(T) m, size_t count);

    Reserves space for at least 'count' items.
    Throws: ITER_EINVAL, ITER_ENOMEM.
*/
#define mapreserve(map, count) map_reserve(map_base(map), (count))

/** Group: Properties */

/** allocator_t *mapallocator(map(T) m);

    Returns the allocator used by 'm'.
*/
#define mapallocator(m) map_allocator(map_base(m))

/** int mapisempty(map(T) map);

    Checks if 'map' is empty.
*/
#define mapisempty(map) map_isempty(map_base(map))

/** size_t mapcount(map(T) map);

    Returns the number of items present in 'map'.
*/
#define mapcount(map) map_count(map_base(map))

/** size_t mapcap(map(T) map);

    Returns the number of slots reserved in 'map'.
*/
#define mapcap(map) map_cap(map_base(map))

/** Group: Main operations */

/** int mapget(map(T) map, const T *key, T *out);

    Copies the item inside 'map' that is equal to the passed 'key',
    which is only used for hashing and comparison.

    Parameters 'key' and 'out' can have identical values.

    Returns: ITER_EINVAL, ITER_ENOENT.
    Throws: ITER_EINVAL.
*/
#define mapget(map, item, out)                                \
    (map_get(map_base(map), map_check(item), map_check(out)))

/** T *mapptr(map(T) map, const T *key);

    Returns the pointer to the item inside 'map' that is equal to the passed
    'key', which is only used for hashing and comparison.

    Returns: `NULL` if not found.
    Throws: ITER_EINVAL.
*/
#define mapptr(map, item) (map_ptr(map_base(map), map_check(item)))

/** int mapset(map(T) map, const T *item);

    Inserts or updates the requested item inside 'map' by copying 'item'.

    Returns/Throws: ITER_EINVAL.
*/
#define mapset(map, item) map_set(map_base(map), map_check(item))

/** int mapinsert(map(T) map, const T *item);

    Inserts the requested item inside 'map' by copying 'item'.

    Returns: ITER_EINVAL, ITER_EEXIST.
    Throws: ITER_EINVAL.
*/
#define mapinsert(map, item) map_insert(map_base(map), map_check(item))

/** int mapupdate(map(T) map, const T *item);

    Updates the requested item inside 'map' by copying 'item'.

    Returns: ITER_EINVAL, ITER_ENOENT.
    Throws: ITER_EINVAL.
*/
#define mapupdate(map, item) map_update(map_base(map), map_check(item))

/** int mapremove(map(T) map, const T *key, T *out);

    Removes the requested item inside 'map'.
    If 'out' is not `NULL`, the removed item is copied to it.

    Returns: ITER_EINVAL, ITER_ENOENT.
    Throws: ITER_EINVAL.
*/
#define mapremove(map, item, out)                     \
    map_remove(map_base(map), map_check(item), (out))

/** int mapeach(map(T) map, map_each_fn *fn, void *user);

    Calls 'fn' for each item in 'map'. If the callback returns a non-zero value,
    the iteration is interrupted and the function exits immediately.

    Returns: ITER_EINVAL, ITER_EINTR.
    Throws: ITER_EINVAL.
*/
#define mapeach(map, fn, user) map_each(map_base(map), (fn), (user))

/** Group: Typeless functions
    Custom-prototype: false
*/

/** Creates a new instance of `map_t`. */
ITER_API map_t *map_new(
    map_hash_fn *hash, allocator_t *allocator, size_t size, size_t align
);

/** Initializes 'out' and returns it. */
ITER_API map_t *map_init(
    map_t *out,
    map_hash_fn *hash,
    allocator_t *allocator,
    size_t size,
    size_t align
);

/** Frees the item buffer and 'map' object. */
ITER_API void map_free(map_t *m);

/** Reserves space for at least 'count' items. */
ITER_API int map_reserve(map_t *m, size_t count);

/** Checks if count is 0. */
ITER_INLINE int map_isempty(map_t *m) { return !m || m->count == 0; }

/** Returns the number of items present. */
ITER_INLINE size_t map_count(map_t *m) { return m ? m->count : 0; }

/** Returns the number of slots reserved. */
ITER_INLINE size_t map_cap(map_t *m) { return m ? m->cap : 0; }

/** Returns allocator used by 'm'. */
ITER_API allocator_t *map_allocator(map_t *m);

/** Searches for item by it's key and copies it to 'out'. */
ITER_API int map_get(map_t *m, const void *key, void *out);

/** Searches for item by it's key and returns it's pointer. */
ITER_API void *map_ptr(map_t *m, const void *key);

/** Inserts or updates item in 'map' by copying. */
ITER_API int map_set(map_t *m, const void *item);

/** Inserts item in 'map' by copying. */
ITER_API int map_insert(map_t *m, const void *item);

/** Updates item in 'map' by copying. */
ITER_API int map_update(map_t *m, const void *item);

/** Removes the item in 'map'. */
ITER_API int map_remove(map_t *m, const void *key, void *out);

/** Calls 'fn' for each item present in 'map'. */
ITER_API int map_each(map_t *map, map_each_fn *fn, void *user);

#endif
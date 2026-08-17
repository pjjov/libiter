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

#endif
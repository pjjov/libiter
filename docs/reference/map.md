<!--
    libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
-->

# map(T) - associative arrays

- [`map_type`](#map_type) — Extracts the item type for a given map.
- [`map_type_ptr`](#map_type_ptr) — Extracts the item type as a pointer for a given map.
- [`map_type_size`](#map_type_size) — Extracts the size of the item type for a given map.
- [`map_base`](#map_base) — Cast the map into `map_t *` with some type safety.
- [`map_check`](#map_check) — Checks if given item has the map's subtype.
- [`map_checkd`](#map_checkd) — Checks if given item has the map's subtype as double pointer.
- [`mapnew`](#mapnew) — Creates a new instance of `map(T)`, allocated with 'allocator'.
- [`mapinit`](#mapinit) — Initializes 'out' to a new instance of `map(T)`, allocated with 'allocator'.
- [`mapfree`](#mapfree) — Frees the item buffer of 'm' and frees 'm' if `mapinit` was not used.
- [`mapreserve`](#mapreserve) — Reserves space for at least 'count' items.
- [`mapallocator`](#mapallocator) — Returns the allocator used by 'm'.
- [`mapisempty`](#mapisempty) — Checks if 'map' is empty.
- [`mapcount`](#mapcount) — Returns the number of items present in 'map'.
- [`mapcap`](#mapcap) — Returns the number of slots reserved in 'map'.
- [`mapget`](#mapget) — Copies the item inside 'map' that is equal to the passed 'key', which is only used for hashing and comparison.
- [`mapptr`](#mapptr) — Returns the pointer to the item inside 'map' that is equal to the passed 'key', which is only used for hashing and comparison.
- [`mapset`](#mapset) — Inserts or updates the requested item inside 'map' by copying 'item'.
- [`mapinsert`](#mapinsert) — Inserts the requested item inside 'map' by copying 'item'.
- [`mapupdate`](#mapupdate) — Updates the requested item inside 'map' by copying 'item'.
- [`mapremove`](#mapremove) — Removes the requested item inside 'map'.
- [`mapeach`](#mapeach) — Calls 'fn' for each item in 'map'.
- [`maphash`](#maphash) — Default hashing function for byte arrays.
- [`map_new`](#map_new) — Creates a new instance of `map_t`.
- [`map_init`](#map_init) — Initializes 'out' and returns it.
- [`map_free`](#map_free) — Frees the item buffer and 'map' object.
- [`map_reserve`](#map_reserve) — Reserves space for at least 'count' items.
- [`map_isempty`](#map_isempty) — Checks if count is 0.
- [`map_count`](#map_count) — Returns the number of items present.
- [`map_cap`](#map_cap) — Returns the number of slots reserved.
- [`map_allocator`](#map_allocator) — Returns allocator used by 'm'.
- [`map_get`](#map_get) — Searches for item by it's key and copies it to 'out'.
- [`map_ptr`](#map_ptr) — Searches for item by it's key and returns it's pointer.
- [`map_set`](#map_set) — Inserts or updates item in 'map' by copying.
- [`map_insert`](#map_insert) — Inserts item in 'map' by copying.
- [`map_update`](#map_update) — Updates item in 'map' by copying.
- [`map_remove`](#map_remove) — Removes the item in 'map'.
- [`map_each`](#map_each) — Calls 'fn' for each item present in 'map'.

## Helper macros

<a id="map_type"></a>

### `map_type`

```c
#define map_type(MAP)
```

Extracts the item type for a given map.

<a id="map_type_ptr"></a>

### `map_type_ptr`

```c
#define map_type_ptr(MAP)
```

Extracts the item type as a pointer for a given map.

<a id="map_type_size"></a>

### `map_type_size`

```c
#define map_type_size(MAP)
```

Extracts the size of the item type for a given map.

<a id="map_base"></a>

### `map_base`

```c
#define map_base(MAP)
```

Cast the map into `map_t *` with some type safety.

<a id="map_check"></a>

### `map_check`

```c
#define map_check(MAP, ITEM)
```

Checks if given item has the map's subtype.

<a id="map_checkd"></a>

### `map_checkd`

```c
#define map_checkd(MAP, ITEM)
```

Checks if given item has the map's subtype as double pointer.

## Allocation functions

<a id="mapnew"></a>

### `mapnew`

```c
map(T) mapnew(type T, allocator_t *allocator);
```

Creates a new instance of `map(T)`, allocated with 'allocator'.

**Returns**

`NULL` if out of memory.

<a id="mapinit"></a>

### `mapinit`

```c
map(T) mapinit(type T, map_t *out, allocator_t *allocator);
```

Initializes 'out' to a new instance of `map(T)`, allocated with 'allocator'.

<a id="mapfree"></a>

### `mapfree`

```c
void mapfree(map(T) m);
```

Frees the item buffer of 'm' and frees 'm' if `mapinit` was not used.

<a id="mapreserve"></a>

### `mapreserve`

```c
int mapreserve(map(T) m, size_t count);
```

Reserves space for at least 'count' items.

**Throws**

ITER_EINVAL, ITER_ENOMEM.

## Properties

<a id="mapallocator"></a>

### `mapallocator`

```c
allocator_t *mapallocator(map(T) m);
```

Returns the allocator used by 'm'.

<a id="mapisempty"></a>

### `mapisempty`

```c
int mapisempty(map(T) map);
```

Checks if 'map' is empty.

<a id="mapcount"></a>

### `mapcount`

```c
size_t mapcount(map(T) map);
```

Returns the number of items present in 'map'.

<a id="mapcap"></a>

### `mapcap`

```c
size_t mapcap(map(T) map);
```

Returns the number of slots reserved in 'map'.

## Main operations

<a id="mapget"></a>

### `mapget`

```c
int mapget(map(T) map, const T *key, T *out);
```

Copies the item inside 'map' that is equal to the passed 'key',
which is only used for hashing and comparison.

Parameters 'key' and 'out' can have identical values.

**Returns**

ITER_EINVAL, ITER_ENOENT.

**Throws**

ITER_EINVAL.

<a id="mapptr"></a>

### `mapptr`

```c
T *mapptr(map(T) map, const T *key);
```

Returns the pointer to the item inside 'map' that is equal to the passed
'key', which is only used for hashing and comparison.

**Returns**

`NULL` if not found.

**Throws**

ITER_EINVAL.

<a id="mapset"></a>

### `mapset`

```c
int mapset(map(T) map, const T *item);
```

Inserts or updates the requested item inside 'map' by copying 'item'.

**Returns**

ITER_EINVAL.

**Throws**

ITER_EINVAL.

<a id="mapinsert"></a>

### `mapinsert`

```c
int mapinsert(map(T) map, const T *item);
```

Inserts the requested item inside 'map' by copying 'item'.

**Returns**

ITER_EINVAL, ITER_EEXIST.

**Throws**

ITER_EINVAL.

<a id="mapupdate"></a>

### `mapupdate`

```c
int mapupdate(map(T) map, const T *item);
```

Updates the requested item inside 'map' by copying 'item'.

**Returns**

ITER_EINVAL, ITER_ENOENT.

**Throws**

ITER_EINVAL.

<a id="mapremove"></a>

### `mapremove`

```c
int mapremove(map(T) map, const T *key, T *out);
```

Removes the requested item inside 'map'.
If 'out' is not `NULL`, the removed item is copied to it.

**Returns**

ITER_EINVAL, ITER_ENOENT.

**Throws**

ITER_EINVAL.

<a id="mapeach"></a>

### `mapeach`

```c
int mapeach(map(T) map, map_each_fn *fn, void *user);
```

Calls 'fn' for each item in 'map'. If the callback returns a non-zero value,
the iteration is interrupted and the function exits immediately.

**Returns**

ITER_EINVAL, ITER_EINTR.

**Throws**

ITER_EINVAL.

## Typeless functions

<a id="maphash"></a>

### `maphash`

```c
ITER_API int maphash(const void *buf, size_t size);
```

Default hashing function for byte arrays.

<a id="map_new"></a>

### `map_new`

```c
ITER_API map_t *map_new(
    map_hash_fn *hash, allocator_t *allocator, size_t size, size_t align
);
```

Creates a new instance of `map_t`.

<a id="map_init"></a>

### `map_init`

```c
ITER_API map_t *map_init(
    map_t *out,
    map_hash_fn *hash,
    allocator_t *allocator,
    size_t size,
    size_t align
);
```

Initializes 'out' and returns it.

<a id="map_free"></a>

### `map_free`

```c
ITER_API void map_free(map_t *m);
```

Frees the item buffer and 'map' object.

<a id="map_reserve"></a>

### `map_reserve`

```c
ITER_API int map_reserve(map_t *m, size_t count);
```

Reserves space for at least 'count' items.

<a id="map_isempty"></a>

### `map_isempty`

```c
ITER_INLINE int map_isempty(map_t *m);
```

Checks if count is 0.

<a id="map_count"></a>

### `map_count`

```c
ITER_INLINE size_t map_count(map_t *m);
```

Returns the number of items present.

<a id="map_cap"></a>

### `map_cap`

```c
ITER_INLINE size_t map_cap(map_t *m);
```

Returns the number of slots reserved.

<a id="map_allocator"></a>

### `map_allocator`

```c
ITER_API allocator_t *map_allocator(map_t *m);
```

Returns allocator used by 'm'.

<a id="map_get"></a>

### `map_get`

```c
ITER_API int map_get(map_t *m, const void *key, void *out);
```

Searches for item by it's key and copies it to 'out'.

<a id="map_ptr"></a>

### `map_ptr`

```c
ITER_API void *map_ptr(map_t *m, const void *key);
```

Searches for item by it's key and returns it's pointer.

<a id="map_set"></a>

### `map_set`

```c
ITER_API int map_set(map_t *m, const void *item);
```

Inserts or updates item in 'map' by copying.

<a id="map_insert"></a>

### `map_insert`

```c
ITER_API int map_insert(map_t *m, const void *item);
```

Inserts item in 'map' by copying.

<a id="map_update"></a>

### `map_update`

```c
ITER_API int map_update(map_t *m, const void *item);
```

Updates item in 'map' by copying.

<a id="map_remove"></a>

### `map_remove`

```c
ITER_API int map_remove(map_t *m, const void *key, void *out);
```

Removes the item in 'map'.

<a id="map_each"></a>

### `map_each`

```c
ITER_API int map_each(map_t *map, map_each_fn *fn, void *user);
```

Calls 'fn' for each item present in 'map'.


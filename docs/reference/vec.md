<!--
    libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
-->

# vec(T) - growable arrays

- [`vec_type`](#vec_type) — Extracts the item type for a given vector.
- [`vec_type_ptr`](#vec_type_ptr) — Extracts the item type as a pointer for a given vector.
- [`vec_type_size`](#vec_type_size) — Extracts the size of the item type for a given vector.
- [`vec_base`](#vec_base) — Cast the vector into `vec_t *` with some type safety.
- [`vec_check`](#vec_check) — Checks if given item has the vector's subtype.
- [`vec_checkd`](#vec_checkd) — Checks if given item has the vector's subtype as double pointer.
- [`VECFORBODY`](#vecforbody) — Expands into a for loop body with the given variable name.
- [`VECFOREACH`](#vecforeach) — Expands into a for loop with the given variable name.
- [`vecnew`](#vecnew) — Creates a new instance of `vec(T)`, allocated with 'allocator'.
- [`vecnew_array`](#vecnew_array) — Creates a new instance of `vec(T)`, allocated with 'allocator'.
- [`vecnew_arrayp`](#vecnew_arrayp) — Creates a new instance of `vec(T)`, allocated with 'allocator'.
- [`vecinit`](#vecinit) — Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
- [`vecinit_array`](#vecinit_array) — Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
- [`vecinit_arrayp`](#vecinit_arrayp) — Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
- [`vecunwrap`](#vecunwrap) — Returns the item buffer of 'v' and frees 'v' if `vecinit` was not used.
- [`vecfree`](#vecfree) — Frees the item buffer of 'v' and frees 'v' if `vecinit` was not used.
- [`vecresize`](#vecresize) — Resizes the item buffer to fit 'cap' items.
- [`vecreserve`](#vecreserve) — Reserves space for at least 'count' items.
- [`veclen`](#veclen) — Returns the number of items present in 'v'.
- [`veccap`](#veccap) — Returns the number of slots allocated in 'v'.
- [`vecallocator`](#vecallocator) — Returns the allocator used by 'v'.
- [`vecslot`](#vecslot) — Returns a pointer to the item slot at index i.
- [`vecptr`](#vecptr) — Returns a pointer to the item at index i.
- [`vecget`](#vecget) — Returns the item at index i.
- [`vecindex`](#vecindex) — Returns the index of item at given pointer.
- [`vecitems`](#vecitems) — Returns the pointer of the allocated item buffer.
- [`vecstart`](#vecstart) — Returns a pointer to the start of the used part of the item buffer.
- [`vecend`](#vecend) — Returns a pointer to the end of the used part of the item buffer.
- [`vecisempty`](#vecisempty) — Checks if length of 'v' is 0.
- [`vecclear`](#vecclear) — Sets the length of 'v' to 0.
- [`vecsetlen`](#vecsetlen) — Sets the length of 'v' to given parameter.
- [`vecsetcap`](#vecsetcap) — Sets the capacity of 'v' to given parameter.
- [`vecincr`](#vecincr) — Increases the vector's length by count and returns the first added slot.
- [`vecdecr`](#vecdecr) — Decreases the vector's length by count and returns the slot at the end of the buffer.
- [`vecinsert`](#vecinsert) — Inserts 'count' items at index 'i'.
- [`vecpush`](#vecpush) — Inserts 'count' items at the end of the item buffer.
- [`vecunshift`](#vecunshift) — Inserts 'count' items at the start of the item buffer.
- [`vecinsertp`](#vecinsertp) — Inserts 'count' items at index 'i', by pointer.
- [`vecfill`](#vecfill) — Inserts 'count' copies of 'item' at index 'i'.
- [`vecremove`](#vecremove) — Removes 'count' items start at index 'i'.
- [`vecpop`](#vecpop) — Removes 'count' items from the end of the item buffer.
- [`vecshift`](#vecshift) — Removes 'count' items from the start of the item buffer.
- [`vecswap`](#vecswap) — Swaps 'count' items starting at indices 'i' and 'j'.
- [`vecswap_remove`](#vecswap_remove) — Removes 'count' items start at index 'i', by swapping items.
- [`veceach`](#veceach) — Calls 'fn' for each item in 'v'.
- [`vec_new`](#vec_new) — Creates a new instance of `vec_t`.
- [`vec_new_array`](#vec_new_array) — Creates a new instance of `vec_t`.
- [`vec_new_arrayp`](#vec_new_arrayp) — Creates a new instance of `vec_t`.
- [`vec_init`](#vec_init) — Initializes 'out' and returns it.
- [`vec_init_array`](#vec_init_array) — Initializes 'out' and returns it.
- [`vec_init_arrayp`](#vec_init_arrayp) — Initializes 'out' and returns it.
- [`vec_unwrap`](#vec_unwrap) — Returns the item buffer and frees 'v'.
- [`vec_free`](#vec_free) — Frees the item buffer and vector 'v'.
- [`vec_len`](#vec_len) — Returns the number of bytes used in 'v'.
- [`vec_cap`](#vec_cap) — Returns the number of bytes allocated in 'v'.
- [`vec_allocator`](#vec_allocator) — Returns allocator used by 'v'.
- [`vec_slot`](#vec_slot) — Returns pointer to the item slot.
- [`vec_ptr`](#vec_ptr) — Returns pointer to the item at given index.
- [`vec_index`](#vec_index) — Returns the item index of given pointer.
- [`vec_items`](#vec_items) — Returns the item buffer.
- [`vec_start`](#vec_start) — Returns the pointer past the first item.
- [`vec_end`](#vec_end) — Returns the pointer past the last item.
- [`vec_isempty`](#vec_isempty) — Checks if length is 0.
- [`vec_clear`](#vec_clear) — Sets the length of 'v' to 0.
- [`vec_setlen`](#vec_setlen) — Naively sets the length of 'v'.
- [`vec_setcap`](#vec_setcap) — Naively sets the capacity of 'v'.
- [`vec_resize`](#vec_resize) — Resizes the item buffer to fit 'cap' items.
- [`vec_reserve`](#vec_reserve) — Reserves space for at least 'count' items.
- [`vec_incr`](#vec_incr) — Increases length by 'count' and returns the first added slot.
- [`vec_decr`](#vec_decr) — Decreases length by 'count' and returns the first removed slot.
- [`vec_insert`](#vec_insert) — Inserts 'count' items start at index 'i'.
- [`vec_push`](#vec_push) — Inserts 'count' items at the end of the item buffer.
- [`vec_unshift`](#vec_unshift) — Inserts 'count' items at the start of the item buffer.
- [`vec_insertp`](#vec_insertp) — Inserts 'count' items starting at index 'i' by reference.
- [`vec_fill`](#vec_fill) — Inserts 'count' copies of 'item' at index 'i'
- [`vec_remove`](#vec_remove) — Removes 'count' items starting at index 'i'.
- [`vec_pop`](#vec_pop) — Removes 'count' items at the end of the item buffer.
- [`vec_shift`](#vec_shift) — Removes 'count' items at the start of the item buffer.
- [`vec_swap`](#vec_swap) — Swaps 'count' items starting at indices 'i' and 'j'.
- [`vec_swap_remove`](#vec_swap_remove) — Removes 'count' items by swapping items.
- [`vec_each`](#vec_each) — Calls 'fn' for each item present in 'v'.

## Helper macros

<a id="vec_type"></a>

### `vec_type`

```c
#define vec_type(VEC)
```

Extracts the item type for a given vector.

<a id="vec_type_ptr"></a>

### `vec_type_ptr`

```c
#define vec_type_ptr(VEC)
```

Extracts the item type as a pointer for a given vector.

<a id="vec_type_size"></a>

### `vec_type_size`

```c
#define vec_type_size(VEC)
```

Extracts the size of the item type for a given vector.

<a id="vec_base"></a>

### `vec_base`

```c
#define vec_base(VEC)
```

Cast the vector into `vec_t *` with some type safety.

<a id="vec_check"></a>

### `vec_check`

```c
#define vec_check(VEC, ITEM)
```

Checks if given item has the vector's subtype.

<a id="vec_checkd"></a>

### `vec_checkd`

```c
#define vec_checkd(VEC, ITEM)
```

Checks if given item has the vector's subtype as double pointer.

<a id="vecforbody"></a>

### `VECFORBODY`

```c
#define VECFORBODY(VEC, VAR)
```

Expands into a for loop body with the given variable name.

<a id="vecforeach"></a>

### `VECFOREACH`

```c
#define VECFOREACH(VEC, VAR)
```

Expands into a for loop with the given variable name.

## Allocation functions

<a id="vecnew"></a>

### `vecnew`

```c
vec(T) vecnew(type T, allocator_t *allocator);
```

Creates a new instance of `vec(T)`, allocated with 'allocator'.

**Returns**

`NULL` if out of memory.

<a id="vecnew_array"></a>

### `vecnew_array`

```c
vec(T) vecnew_array(T *array, size_t length, allocator_t *allocator);
```

Creates a new instance of `vec(T)`, allocated with 'allocator'.
The content of 'array' is copied to the resulting vector.

<a id="vecnew_arrayp"></a>

### `vecnew_arrayp`

```c
vec(T) vecnew_arrayp(T **array, size_t length, allocator_t *allocator);
```

Creates a new instance of `vec(T)`, allocated with 'allocator'.
The content of 'array' is copied to the resulting vector.

<a id="vecinit"></a>

### `vecinit`

```c
vec(T) vecinit(type T, vec_t *out, allocator_t *allocator);
```

Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.

<a id="vecinit_array"></a>

### `vecinit_array`

```c
vec(T) vecinit_array(T *array, size_t length, vec_t *out, allocator_t *allocator);
```

Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
The content of 'array' is copied to the resulting vector.

<a id="vecinit_arrayp"></a>

### `vecinit_arrayp`

```c
vec(T) vecinit_arrayp(T **array, size_t length, vec_t *out, allocator_t *allocator);
```

Initializes 'out' to a new instance of `vec(T)`, allocated with 'allocator'.
The content of 'array' is copied to the resulting vector.

<a id="vecunwrap"></a>

### `vecunwrap`

```c
T *vecunwrap(vec(T) v);
```

Returns the item buffer of 'v' and frees 'v' if `vecinit` was not used.

<a id="vecfree"></a>

### `vecfree`

```c
void vecfree(vec(T) v);
```

Frees the item buffer of 'v' and frees 'v' if `vecinit` was not used.

<a id="vecresize"></a>

### `vecresize`

```c
int vecresize(vec(T) v, size_t cap);
```

Resizes the item buffer to fit 'cap' items.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecreserve"></a>

### `vecreserve`

```c
int vecreserve(vec(T) v, size_t count);
```

Reserves space for at least 'count' items.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

## Properties

<a id="veclen"></a>

### `veclen`

```c
size_t veclen(vec(T) v);
```

Returns the number of items present in 'v'.

<a id="veccap"></a>

### `veccap`

```c
size_t veccap(vec(T) v);
```

Returns the number of slots allocated in 'v'.

<a id="vecallocator"></a>

### `vecallocator`

```c
allocator_t *vecallocator(vec(T) v);
```

Returns the allocator used by 'v'.

<a id="vecslot"></a>

### `vecslot`

```c
T *vecslot(vec(T) v, size_t i);
```

Returns a pointer to the item slot at index i.

<a id="vecptr"></a>

### `vecptr`

```c
T *vecptr(vec(T) v, size_t i);
```

Returns a pointer to the item at index i.

<a id="vecget"></a>

### `vecget`

```c
T vecget(vec(T) v, size_t i);
```

Returns the item at index i.

<a id="vecindex"></a>

### `vecindex`

```c
size_t vecindex(vec(T) v, T *item);
```

Returns the index of item at given pointer.

**Throws**

ITER_EINVAL.

<a id="vecitems"></a>

### `vecitems`

```c
T *vecitems(vec(T) v);
```

Returns the pointer of the allocated item buffer.

<a id="vecstart"></a>

### `vecstart`

```c
T *vecstart(vec(T) v);
```

Returns a pointer to the start of the used part of the item buffer.

<a id="vecend"></a>

### `vecend`

```c
T *vecend(vec(T) v);
```

Returns a pointer to the end of the used part of the item buffer.

<a id="vecisempty"></a>

### `vecisempty`

```c
int vecisempty(vec(T) v);
```

Checks if length of 'v' is 0.

<a id="vecclear"></a>

### `vecclear`

```c
void vecclear(vec(T) v);
```

Sets the length of 'v' to 0.

<a id="vecsetlen"></a>

### `vecsetlen`

```c
void vecsetlen(vec(T) v, size_t len);
```

Sets the length of 'v' to given parameter.

<a id="vecsetcap"></a>

### `vecsetcap`

```c
void vecsetcap(vec(T) v, size_t cap);
```

Sets the capacity of 'v' to given parameter.

## Main operations

<a id="vecincr"></a>

### `vecincr`

```c
T *vecincr(vec(T) v, size_t count);
```

Increases the vector's length by count and returns the first added slot.
This function is an inverted variant of 'push', allowing the user to
initialize an object instead of copying it.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecdecr"></a>

### `vecdecr`

```c
T *vecdecr(vec(T) v, size_t count);
```

Decreases the vector's length by count and returns the slot at the end of
the buffer. This function is an inverted variant of 'pop', allowing the
user to free an object after removing it.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecinsert"></a>

### `vecinsert`

```c
int vecinsert(vec(T) v, T *items, size_t i, size_t count);
```

Inserts 'count' items at index 'i'.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecpush"></a>

### `vecpush`

```c
int vecpush(vec(T) v, T *items, size_t count);
```

Inserts 'count' items at the end of the item buffer.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecunshift"></a>

### `vecunshift`

```c
int vecunshift(vec(T) v, T *items, size_t count);
```

Inserts 'count' items at the start of the item buffer.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecinsertp"></a>

### `vecinsertp`

```c
int vecinsertp(vec(T) v, T **items, size_t i, size_t count);
```

Inserts 'count' items at index 'i', by pointer.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecfill"></a>

### `vecfill`

```c
int vecfill(vec(T) v, T *item, size_t i, size_t count);
```

Inserts 'count' copies of 'item' at index 'i'.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecremove"></a>

### `vecremove`

```c
int vecremove(vec(T) v, T *out, size_t i, size_t count);
```

Removes 'count' items start at index 'i'.
If 'out' is not `NULL`, removed items are copied to it.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecpop"></a>

### `vecpop`

```c
int vecpop(vec(T) v, T *out, size_t count);
```

Removes 'count' items from the end of the item buffer.
If 'out' is not `NULL`, removed items are copied to it.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecshift"></a>

### `vecshift`

```c
int vecshift(vec(T) v, T *out, size_t count);
```

Removes 'count' items from the start of the item buffer.
If 'out' is not `NULL`, removed items are copied to it.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecswap"></a>

### `vecswap`

```c
int vecswap(vec(T) v, size_t i, size_t j, size_t count);
```

Swaps 'count' items starting at indices 'i' and 'j'.
> Space past the last item is used as a temporary buffer.

**Throws**

ITER_EINVAL, ITER_ENOMEM, ITER_EOVERFLOW.

<a id="vecswap_remove"></a>

### `vecswap_remove`

```c
int vecswap_remove(vec(T) v, T *out, size_t i, size_t count);
```

Removes 'count' items start at index 'i', by swapping items.
If 'out' is not `NULL`, removed items are copied to it.

**Throws**

ITER_EINVAL, ITER_EOVERFLOW.

## Iteration functions

<a id="veceach"></a>

### `veceach`

```c
int veceach(vec(T) v, vec_each_fn *fn, void *user);
```

Calls 'fn' for each item in 'v'. If the callback returns a non-zero value,
the iteration is interrupted and the function exits immediately.

**Returns**

ITER_EINVAL, ITER_EINTR.

**Throws**

ITER_EINVAL.

## Typeless functions

<a id="vec_new"></a>

### `vec_new`

```c
ITER_API vec_t *vec_new(allocator_t *allocator);
```

Creates a new instance of `vec_t`.

<a id="vec_new_array"></a>

### `vec_new_array`

```c
ITER_API vec_t *vec_new_array(
    void *array, size_t length, allocator_t *allocator, size_t size
);
```

Creates a new instance of `vec_t`. Copies the content of 'array'.

<a id="vec_new_arrayp"></a>

### `vec_new_arrayp`

```c
ITER_API vec_t *vec_new_arrayp(
    void **array, size_t length, allocator_t *allocator, size_t size
);
```

Creates a new instance of `vec_t`. Copies the content of 'array'.

<a id="vec_init"></a>

### `vec_init`

```c
ITER_API vec_t *vec_init(vec_t *out, allocator_t *allocator);
```

Initializes 'out' and returns it.

<a id="vec_init_array"></a>

### `vec_init_array`

```c
ITER_API vec_t *vec_init_array(
    void *array, size_t length, vec_t *out, allocator_t *allocator, size_t size
);
```

Initializes 'out' and returns it. Copies the content of 'array'.

<a id="vec_init_arrayp"></a>

### `vec_init_arrayp`

```c
ITER_API vec_t *vec_init_arrayp(
    void **array, size_t length, vec_t *out, allocator_t *allocator, size_t size
);
```

Initializes 'out' and returns it. Copies the content of 'array'.

<a id="vec_unwrap"></a>

### `vec_unwrap`

```c
ITER_API void *vec_unwrap(vec_t *v);
```

Returns the item buffer and frees 'v'.

<a id="vec_free"></a>

### `vec_free`

```c
ITER_API void vec_free(vec_t *v);
```

Frees the item buffer and vector 'v'.

<a id="vec_len"></a>

### `vec_len`

```c
ITER_INLINE size_t vec_len(vec_t *v, size_t size);
```

Returns the number of bytes used in 'v'.

<a id="vec_cap"></a>

### `vec_cap`

```c
ITER_INLINE size_t vec_cap(vec_t *v, size_t size);
```

Returns the number of bytes allocated in 'v'.

<a id="vec_allocator"></a>

### `vec_allocator`

```c
ITER_API allocator_t *vec_allocator(vec_t *v);
```

Returns allocator used by 'v'.

<a id="vec_slot"></a>

### `vec_slot`

```c
ITER_INLINE void *vec_slot(vec_t *v, size_t i, size_t size);
```

Returns pointer to the item slot.

<a id="vec_ptr"></a>

### `vec_ptr`

```c
ITER_INLINE void *vec_ptr(vec_t *v, size_t i, size_t size);
```

Returns pointer to the item at given index.

<a id="vec_index"></a>

### `vec_index`

```c
ITER_API size_t vec_index(vec_t *v, void *item, size_t size);
```

Returns the item index of given pointer.

<a id="vec_items"></a>

### `vec_items`

```c
ITER_INLINE void *vec_items(vec_t *v);
```

Returns the item buffer.

<a id="vec_start"></a>

### `vec_start`

```c
ITER_INLINE void *vec_start(vec_t *v);
```

Returns the pointer past the first item.

<a id="vec_end"></a>

### `vec_end`

```c
ITER_INLINE void *vec_end(vec_t *v);
```

Returns the pointer past the last item.

<a id="vec_isempty"></a>

### `vec_isempty`

```c
ITER_INLINE int vec_isempty(vec_t *v);
```

Checks if length is 0.

<a id="vec_clear"></a>

### `vec_clear`

```c
ITER_INLINE void vec_clear(vec_t *v);
```

Sets the length of 'v' to 0.

<a id="vec_setlen"></a>

### `vec_setlen`

```c
ITER_INLINE void vec_setlen(vec_t *v, size_t len, size_t size);
```

Naively sets the length of 'v'.

<a id="vec_setcap"></a>

### `vec_setcap`

```c
ITER_INLINE void vec_setcap(vec_t *v, size_t cap, size_t size);
```

Naively sets the capacity of 'v'.

<a id="vec_resize"></a>

### `vec_resize`

```c
ITER_API int vec_resize(vec_t *v, size_t cap, size_t size);
```

Resizes the item buffer to fit 'cap' items.

<a id="vec_reserve"></a>

### `vec_reserve`

```c
ITER_API int vec_reserve(vec_t *v, size_t count, size_t size);
```

Reserves space for at least 'count' items.

<a id="vec_incr"></a>

### `vec_incr`

```c
ITER_API void *vec_incr(vec_t *v, size_t count, size_t size);
```

Increases length by 'count' and returns the first added slot.

<a id="vec_decr"></a>

### `vec_decr`

```c
ITER_API void *vec_decr(vec_t *v, size_t count, size_t size);
```

Decreases length by 'count' and returns the first removed slot.

<a id="vec_insert"></a>

### `vec_insert`

```c
ITER_API int vec_insert(
    vec_t *v, void *items, size_t i, size_t count, size_t size
);
```

Inserts 'count' items start at index 'i'.

<a id="vec_push"></a>

### `vec_push`

```c
ITER_API int vec_push(vec_t *v, void *items, size_t count, size_t size);
```

Inserts 'count' items at the end of the item buffer.

<a id="vec_unshift"></a>

### `vec_unshift`

```c
ITER_INLINE int vec_unshift(vec_t *v, void *items, size_t count, size_t size);
```

Inserts 'count' items at the start of the item buffer.

<a id="vec_insertp"></a>

### `vec_insertp`

```c
ITER_API int vec_insertp(
    vec_t *v, void **items, size_t i, size_t count, size_t size
);
```

Inserts 'count' items starting at index 'i' by reference.

<a id="vec_fill"></a>

### `vec_fill`

```c
ITER_API int vec_fill(
    vec_t *v, void *item, size_t i, size_t count, size_t size
);
```

Inserts 'count' copies of 'item' at index 'i'

<a id="vec_remove"></a>

### `vec_remove`

```c
ITER_API int vec_remove(
    vec_t *v, void *out, size_t i, size_t count, size_t size
);
```

Removes 'count' items starting at index 'i'.

<a id="vec_pop"></a>

### `vec_pop`

```c
ITER_API int vec_pop(vec_t *v, void *out, size_t count, size_t size);
```

Removes 'count' items at the end of the item buffer.

<a id="vec_shift"></a>

### `vec_shift`

```c
ITER_INLINE int vec_shift(vec_t *v, void *out, size_t count, size_t size);
```

Removes 'count' items at the start of the item buffer.

<a id="vec_swap"></a>

### `vec_swap`

```c
ITER_API int vec_swap(vec_t *v, size_t i, size_t j, size_t count, size_t size);
```

Swaps 'count' items starting at indices 'i' and 'j'.

<a id="vec_swap_remove"></a>

### `vec_swap_remove`

```c
ITER_API int vec_swap_remove(
    vec_t *v, void *out, size_t i, size_t count, size_t size
);
```

Removes 'count' items by swapping items.

<a id="vec_each"></a>

### `vec_each`

```c
ITER_API int vec_each(vec_t *v, vec_each_fn *fn, void *user, size_t size);
```

Calls 'fn' for each item present in 'v'.


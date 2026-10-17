<!--
    libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
-->

# Generic container macros and type definitions.

- [`generic_container`](#generic_container) — Declares a type generic container with given types
- [`generic_item_type`](#generic_item_type) — Extracts the item type of the given container.
- [`generic_item_ptr`](#generic_item_ptr) — Extracts the item type with a pointer of the given container.
- [`generic_item_dptr`](#generic_item_dptr) — Extracts the item type with a double pointer of the given container.
- [`generic_item_size`](#generic_item_size) — Extracts the item type with a pointer of the given container.
- [`generic_check_item`](#generic_check_item) — Check if given item's type matches container's subtype.
- [`generic_check_item_d`](#generic_check_item_d) — Check if given double-pointer item's type matches container's subtype.
- [`generic_check_container`](#generic_check_container) — Check if given container is of the given type (not subtype).

<a id="generic_container"></a>

## `generic_container`

```c
#define generic_container(T_CNTR, T_ITEM)
```

Declares a type generic container with given types

Parameters:
- T_CNTR - Type of the container.
- T_ITEM - Type of the item.

<a id="generic_item_type"></a>

## `generic_item_type`

```c
#define generic_item_type(T_CNTR, CNTR)
```

Extracts the item type of the given container.

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.

<a id="generic_item_ptr"></a>

## `generic_item_ptr`

```c
#define generic_item_ptr(T_CNTR, CNTR)
```

Extracts the item type with a pointer of the given container.

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.

<a id="generic_item_dptr"></a>

## `generic_item_dptr`

```c
#define generic_item_dptr(T_CNTR, CNTR)
```

Extracts the item type with a double pointer of the given container.

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.

<a id="generic_item_size"></a>

## `generic_item_size`

```c
#define generic_item_size(T_CNTR, CNTR)
```

Extracts the item type with a pointer of the given container.

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.

<a id="generic_check_item"></a>

## `generic_check_item`

```c
#define generic_check_item(T_CNTR, CNTR, ITEM)
```

Check if given item's type matches container's subtype.

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.
- ITEM - Pointer to the given item.

<a id="generic_check_item_d"></a>

## `generic_check_item_d`

```c
#define generic_check_item_d(T_CNTR, CNTR, ITEM)
```

Check if given double-pointer item's type matches container's subtype.

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.
- ITEM - Pointer to the given item.

<a id="generic_check_container"></a>

## `generic_check_container`

```c
#define generic_check_container(T_CNTR, CNTR)
```

Check if given container is of the given type (not subtype).

Parameters:
- T_CNTR - Explicit type of the container, or <void>.
- CNTR - Pointer to the actual container.


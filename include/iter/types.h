/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_TYPES_H

    #ifndef LIBITER_TYPES_ONCE
        #define LIBITER_TYPES_ONCE
        #include <iter/pf_types.h>
        #include <stddef.h>
        #include <stdint.h>

        #ifndef ITER_HASH_TYPE
            #include <limits.h>
            #define ITER_HASH_TYPE size_t
            #define ITER_HASH_MIN SIZE_MIN
            #define ITER_HASH_MAX SIZE_MAX
        #endif

enum libiter_bool {
    ITER_TRUE = 1,
    ITER_FALSE = 0,
};

enum libiter_error {
    ITER_OK = 0,
    ITER_ENOENT = -2,
    ITER_EINTR = -4,
    ITER_EAGAIN = -11,
    ITER_ENOMEM = -12,
    ITER_EEXIST = -17,
    ITER_EINVAL = -22,
    ITER_ENOLCK = -37,
    ITER_ENOSYS = -38,
    ITER_ENODATA = -61,
    ITER_EOVERFLOW = -75,
    ITER_ETIMEDOUT = -110,
    ITER_ECANCELED = -125,
    ITER_EASYNC = -192,
};

typedef struct allocator_t allocator_t;

    #endif

    #ifndef LIBITER_GENERIC
        #define LIBITER_GENERIC

        /** Declares a type generic container with given types

            Parameters:
            - T_CNTR - Type of the container.
            - T_ITEM - Type of the item.
        */
        #define generic_container(T_CNTR, T_ITEM)     \
            typeof(T_ITEM * (**)(T_CNTR * container))

        /** Extracts the item type of the given container.

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
        */
        #define generic_item_type(T_CNTR, CNTR) typeof(*(*(CNTR))((T_CNTR *)0))

        /** Extracts the item type with a pointer of the given container.

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
        */
        #define generic_item_ptr(T_CNTR, CNTR) typeof((*(CNTR))((T_CNTR *)0))

        /** Extracts the item type with a double pointer of the given container.

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
        */
        #define generic_item_dptr(T_CNTR, CNTR)      \
            typeof(typeof((*(CNTR))((T_CNTR *)0)) *)

        /** Extracts the item type with a pointer of the given container.

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
        */
        #define generic_item_size(T_CNTR, CNTR)     \
            sizeof(generic_item_type(T_CNTR, CNTR))

        /** Check if given item's type matches container's subtype.

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
            - ITEM - Pointer to the given item.
        */
        #define generic_check_item(T_CNTR, CNTR, ITEM)          \
            pf_check_type(generic_item_ptr(T_CNTR, CNTR), ITEM)

        /** Check if given double-pointer item's type matches container's subtype.

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
            - ITEM - Pointer to the given item.
        */
        #define generic_check_item_d(T_CNTR, CNTR, ITEM)         \
            pf_check_type(generic_item_dptr(T_CNTR, CNTR), ITEM)

        /** Check if given container is of the given type (not subtype).

            Parameters:
            - T_CNTR - Explicit type of the container, or <void>.
            - CNTR - Pointer to the actual container.
        */
        #define generic_check_container(T_CNTR, CNTR)                       \
            ((T_CNTR *)pf_check_type(                                       \
                generic_container(T_CNTR, generic_item_type(T_CNTR, CNTR)), \
                CNTR                                                        \
            ))

    #endif

    #ifndef LIBITER_NEED_TYPE
        #define LIBITER_NEED_VEC
        #define LIBITER_NEED_MAP
    #else
        #undef LIBITER_NEED_TYPE
    #endif

    #if defined(LIBITER_NEED_VEC) && !defined(LIBITER_HAS_VEC)

        #define LIBITER_HAS_VEC
        #define vec(T) generic_container(vec_t, T)

typedef int(vec_each_fn)(void *item, void *user);

typedef struct vec_t {
    void *items;
    size_t len;
    size_t cap;
    allocator_t *alloc;
} vec_t;

    #endif

    #if defined(LIBITER_NEED_MAP) && !defined(LIBITER_HAS_MAP)

        #define LIBITER_HAS_MAP
        #define map(T) generic_container(map_t, T)

typedef ITER_HASH_TYPE map_hash_t;
typedef map_hash_t(map_hash_fn)(const void *item, const void *other);
typedef int(map_each_fn)(void *item, void *user);

typedef struct map_t {
    size_t cap;
    size_t count;
    void *meta;

    size_t storageTop;
    size_t itemSize;
    size_t itemAlign;
    void *items;

    map_hash_fn *hash;

    size_t bufferSize;
    void *buffer;
    allocator_t *alloc;
} map_t;

    #endif

    #if defined(LIBITER_HAS_VEC) && defined(LIBITER_HAS_MAP)
        #define LIBITER_TYPES_H
    #endif

#endif
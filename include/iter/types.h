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

        #ifndef ITER_HASH_TYPE
            #include <limits.h>
            #define ITER_HASH_TYPE size_t
            #define HASH_MIN SIZE_MIN
            #define HASH_MAX SIZE_MAX
        #endif

typedef struct allocator_t allocator_t;

typedef ITER_HASH_TYPE hash_t;
typedef hash_t(hasher_fn)(const void *buffer, size_t length);
typedef hash_t(hash_fn)(const void *item, const void *other, hasher_fn *hasher);

    #endif

    #ifndef LIBITER_GENERIC
        #define LIBITER_GENERIC

        /** Declares a type generic container with given types

            Parameters:
                T_CNTR - Type of the container.
                T_ITEM - Type of the item.
        */
        #define generic_container(T_CNTR, T_ITEM)     \
            typeof(T_ITEM * (**)(T_CNTR * container))

        /** Extracts the item type of the given container.

        Parameters:
            T_CNTR - Explicit type of the container, or <void>.
            CNTR - Pointer to the actual container.
    */
        #define generic_item_type(T_CNTR, CNTR) typeof(*(*(CNTR))((T_CNTR *)0))

        /** Extracts the item type with a pointer of the given container.

        Parameters:
            T_CNTR - Explicit type of the container, or <void>.
            CNTR - Pointer to the actual container.
    */
        #define generic_item_ptr(T_CNTR, CNTR) typeof((*(CNTR))((T_CNTR *)0, 0))

        /** Extracts the item type with a pointer of the given container.

        Parameters:
            T_CNTR - Explicit type of the container, or <void>.
            CNTR - Pointer to the actual container.
    */
        #define generic_item_size(T_CNTR, CNTR)     \
            sizeof(generic_item_type(T_CNTR, CNTR))

        /** Check if given item's type matches container's subtype.

        Parameters:
            T_CNTR - Explicit type of the container, or <void>.
            CNTR - Pointer to the actual container.
            ITEM - Pointer to the given item.
    */
        #define generic_check_item(T_CNTR, CNTR, ITEM)           \
            pf_check_type(generic_value_ptr(T_CNTR, CNTR), ITEM)

    /** Check if given container is of the given type (not subtype).

        Parameters:
            T_CNTR - Explicit type of the container, or <void>.
            CNTR - Pointer to the actual container.
    */
        #define generic_check_container(T_CNTR, CNTR)                        \
            ((T_CNTR *)pf_check_type(                                        \
                generic_container(T_CNTR, generic_value_type(T_CNTR, CNTR)), \
                CNTR                                                         \
            ))

    #endif

    #ifndef LIBITER_NEED_TYPE
        #define LIBITER_NEED_VEC
    #else
        #undef LIBITER_NEED_TYPE
    #endif

    #if defined(LIBITER_NEED_VEC) && !defined(LIBITER_HAS_VEC)

        #define LIBITER_HAS_VEC
        #define vec(T) generic_container(vec_t, T)

typedef struct vec_t {
    void *items;
    size_t length;
    size_t capacity;
    allocator_t *allocator;
} vec_t;

    #endif

    #if defined(LIBITER_HAS_VEC)
        #define LIBITER_TYPES_H
    #endif

#endif
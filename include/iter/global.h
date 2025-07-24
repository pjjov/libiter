/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_GLOBAL_H
#define LIBITER_GLOBAL_H

#include <allocator.h>
#include <iter/hash.h>

/** # Default allocator

    This function sets the default allocator for containers and returns the
    previous one. Containers created before calling this function will continue
    to use the previously set allocator. If `NULL` is passed, the original
    allocator will be used again.

    **This function is NOT thread-safe.**
**/
allocator_t *libiter_use_allocator(allocator_t *allocator);

/** # Default hasher
    
    This function sets the default hasher for containers and returns the
    previous one. Containers created before calling this function will continue
    to use the previously set hasher. If `NULL` is passed, the original
    hasher will be used again.

    **This function is NOT thread-safe.**
**/
hasher_fn *libiter_use_hasher(hasher_fn *hasher);

#endif

/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <allocator.h>
#include <allocator_std.h>
#include <iter/hash.h>

allocator_t *libiter_allocator = &standard_allocator;
hasher_fn *libiter_hasher = &hasher_fnv1a;

allocator_t *libiter_use_allocator(allocator_t *allocator) {
    if (!allocator)
        allocator = &standard_allocator;

    allocator_t *prev = libiter_allocator;
    libiter_allocator = allocator;
    return prev;
}

hasher_fn *libiter_use_hasher(hasher_fn *hasher) {
    if (!hasher)
        hasher = &hasher_fnv1a;

    hasher_fn *prev = libiter_hasher;
    libiter_hasher = hasher;
    return prev;
}

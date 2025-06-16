/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <allocator.h>
#include <allocator_std.h>

allocator_t *libiter_allocator = &standard_allocator;

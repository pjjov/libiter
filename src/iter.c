/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/iter.h>
#include <pf_overflow.h>
#include <stdint.h>
#include <string.h>

size_t iter__to_array(iter_t *it, void *out, size_t length, size_t stride) {
    if (!it || !out || stride == 0)
        return 0;

    for (size_t i = 0; i < length; i++) {
        void *slot = (void *)((uintptr_t)out + stride * i);
        if (iter__call(it, slot, stride, 0))
            return i;
    }

    return length;
}

static int array_iter_fn(iter_t *it, void *out, size_t size, size_t skip) {
    if (!it || size == 0 || it == out)
        return ITER_EINVAL;

    if (skip) {
        uintptr_t current = (uintptr_t)it->current;
        current += size * skip;
        it->current = (void *)current;

        if (it->current >= it->container)
            return ITER_ENODATA;
    }

    if (out) {
        void *next = (void *)((uintptr_t)it->current + size);
        if (next > it->container)
            return ITER_ENODATA;

        memcpy(out, it->current, size);
        it->current = next;
    }
    return ITER_OK;
}

iter_t *iter__from_array(iter_t *out, const void *items, size_t length) {
    if (!out || !items)
        return NULL;

    out->call = &array_iter_fn;
    out->container = (void *)((uintptr_t)items + length);
    out->bucket = items;
    out->current = items;
    return out;
}

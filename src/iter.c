/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/iter.h>
#include <pf_macro.h>
#include <stdint.h>
#include <string.h>

struct array_iter {
    const void *items;
    size_t current;
    size_t end;
};

size_t iter__to_array(iter_t *it, void *out, size_t length, size_t stride) {
    if (!it || !out || stride == 0)
        return 0;

    for (size_t i = 0; i < length; i++) {
        void *slot = PF_OFFSET(out, stride * i);
        if (iter__call(it, slot, stride, 0))
            return i;
    }

    return length;
}

static int array_iter_fn(iter_t *it, void *out, size_t size, size_t skip) {
    if (!it || size == 0 || it == out)
        return ITER_EINVAL;

    struct array_iter *ait = ITER__CAST(it);

    if (skip) {
        ait->current += size * skip;

        if (ait->current >= ait->end)
            return ITER_ENODATA;
    }

    if (out) {
        if (ait->current + size > ait->end)
            return ITER_ENODATA;

        memcpy(out, PF_OFFSET(ait->items, ait->current), size);
        ait->current += size;
    }
    return ITER_OK;
}

iter_t *iter__from_array(iter_t *out, const void *items, size_t length) {
    if (!out || !items)
        return NULL;

    struct array_iter *ait = ITER__CAST(out);

    out->call = &array_iter_fn;
    ait->items = items;
    ait->current = 0;
    ait->end = length;
    return out;
}

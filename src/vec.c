/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#include "iter/types.h"
#include <iter/vec.h>

#include <allocator.h>
#include <pf_macro.h>
#include <stdint.h>

extern allocator_t *libiter_allocator;

static inline int can_heap_alloc(allocator_t *alloc) {
    return (((uintptr_t)alloc) & 1) == 0;
}

static inline int is_heap_alloc(vec_t *v) {
    return (((uintptr_t)v->alloc) & 1) == 1;
}

static inline void mark_heap_alloc(vec_t *out, allocator_t *alloc) {
    out->alloc = (void *)(((uintptr_t)alloc) | 1);
}

allocator_t *vec_allocator(vec_t *v) {
    if (!v)
        return NULL;

    uintptr_t mask = ~(uintptr_t)1;
    uintptr_t masked = ((uintptr_t)v->alloc) & mask;
    return (allocator_t *)(masked);
}

vec_t *vec_new(allocator_t *alloc) {
    if (!can_heap_alloc(alloc))
        return NULL;

    if (!alloc)
        alloc = libiter_allocator;

    vec_t *out;

    if (!(out = allocate(alloc, sizeof(*out))))
        return NULL;

    vec_init(out, alloc);
    mark_heap_alloc(out, alloc);
    return out;
}

vec_t *vec_init(vec_t *out, allocator_t *alloc) {
    if (!out)
        return NULL;

    if (!alloc)
        alloc = libiter_allocator;

    out->alloc = alloc;
    out->len = 0;
    out->cap = 0;
    return out;
}

void *vec_unwrap(vec_t *v) {
    if (!v)
        return NULL;

    void *out = v->items;

    if (is_heap_alloc(v)) {
        allocator_t *alloc = vec_allocator(v);
        deallocate(alloc, v, sizeof(*v));
    }

    return out;
}

void vec_free(vec_t *v) {
    if (!v)
        return;

    allocator_t *alloc = vec_allocator(v);

    deallocate(alloc, v->items, v->cap);

    if (is_heap_alloc(v))
        deallocate(alloc, v, sizeof(*v));
}
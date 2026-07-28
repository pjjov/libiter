/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#include "iter/types.h"
#include <iter/global.h>
#include <iter/vec.h>

#include <allocator.h>
#include <pf_macro.h>
#include <stdint.h>
#include <string.h>

#define VEC_GROWTH(old, req) ((old + req) * 1.5)
#define VEC_OFFSET(v, i) PF_OFFSET(v->items, i)

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
        return ITER_THROW_NULL(ITER_EINVAL);

    if (!alloc)
        alloc = libiter_allocator;

    vec_t *out;

    if (!(out = allocate(alloc, sizeof(*out))))
        return ITER_THROW_NULL(ITER_ENOMEM);

    vec_init(out, alloc);
    mark_heap_alloc(out, alloc);
    return out;
}

vec_t *vec_init(vec_t *out, allocator_t *alloc) {
    if (!out)
        return ITER_THROW_NULL(ITER_EINVAL);

    if (!alloc)
        alloc = libiter_allocator;

    out->alloc = alloc;
    out->len = 0;
    out->cap = 0;
    return out;
}

vec_t *vec_new_array(
    void *array, size_t length, allocator_t *allocator, size_t size
) {
    vec_t *out;

    if (!(out = vec_new(allocator)))
        return NULL;

    if (vec_insert(out, array, 0, length, size)) {
        vec_free(out);
        return NULL;
    }

    return out;
}

vec_t *vec_init_array(
    void *array, size_t length, vec_t *out, allocator_t *allocator, size_t size
) {
    vec_init(out, allocator);

    if (vec_insert(out, array, 0, length, size)) {
        vec_free(out);
        return NULL;
    }

    return out;
}

vec_t *vec_new_arrayp(
    void **array, size_t length, allocator_t *allocator, size_t size
) {
    vec_t *out;

    if (!(out = vec_new(allocator)))
        return NULL;

    if (vec_insertp(out, array, 0, length, size)) {
        vec_free(out);
        return NULL;
    }

    return out;
}

vec_t *vec_init_arrayp(
    void **array, size_t length, vec_t *out, allocator_t *allocator, size_t size
) {
    vec_init(out, allocator);

    if (vec_insertp(out, array, 0, length, size)) {
        vec_free(out);
        return NULL;
    }

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

size_t vec_index(vec_t *v, void *item, size_t size) {
    if (!v || !item) {
        ITER_THROW_EINVAL;
        return 0;
    }

    size_t last = v->len / size;
    size_t diff = (uint8_t *)item - (uint8_t *)v->items;

    if (item < v->items || item > vec_slot(v, last, size)) {
        ITER_THROW_EINVAL;
        return 0;
    }

    if (diff % size != 0) {
        ITER_THROW_EINVAL;
        return 0;
    }

    return diff / size;
}

int vec_resize(vec_t *v, size_t cap, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;
    if (vec__mul(cap, size))
        return ITER_THROW_EOVERFLOW;

    allocator_t *alloc = vec_allocator(v);
    size_t newCap = cap * size;

    if (!alloc)
        return ITER_THROW_EINVAL;

    void *items = reallocate(alloc, v->items, v->cap, newCap);

    if (!items && newCap > 0)
        return ITER_THROW_ENOMEM;

    v->items = items;
    v->cap = newCap;
    if (v->len > newCap)
        v->len = newCap;

    return ITER_OK;
}

int vec_reserve(vec_t *v, size_t count, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;
    if (vec__mul(count, size))
        return ITER_THROW_EOVERFLOW;

    size_t req = count * size;
    if (v->len + req <= v->cap)
        return ITER_OK;

    size_t newCap = VEC_GROWTH(v->cap, req);
    return vec_resize(v, newCap, 1);
}

static int reserve_region(vec_t *v, size_t i, size_t count, size_t size) {
    if (vec__mul(i, size) || vec__mul(count, size))
        return ITER_THROW_EOVERFLOW;

    size_t off = i * size;

    if (off > v->len)
        return ITER_THROW_EINVAL;

    if (vec_reserve(v, count, size))
        return ITER_ENOMEM;

    if (off < v->len) {
        void *dst = VEC_OFFSET(v, off + size);
        void *src = VEC_OFFSET(v, off);
        memmove(dst, src, v->len - off);
    }

    return ITER_OK;
}

int vec_insert(vec_t *v, void *items, size_t i, size_t count, size_t size) {
    if (!v || !items || size == 0)
        return ITER_THROW_EINVAL;

    int rc;

    if ((rc = reserve_region(v, i, count, size)))
        return rc;

    v->len += count * size;
    memcpy(VEC_OFFSET(v, i * size), items, count * size);
    return ITER_OK;
}

int vec_push(vec_t *v, void *items, size_t count, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;
    return vec_insert(v, items, v->len / size, count, size);
}

int vec_fill(vec_t *v, void *item, size_t i, size_t count, size_t size) {
    if (!v || !item || size == 0)
        return ITER_THROW_EINVAL;

    int rc;

    if ((rc = reserve_region(v, i, count, size)))
        return rc;

    v->len += count * size;
    for (size_t j = 0; j < count; j++)
        memcpy(VEC_OFFSET(v, (i + j) * size), item, size);
    return ITER_OK;
}

int vec_insertp(vec_t *v, void **items, size_t i, size_t count, size_t size) {
    if (!v || !items || size == 0)
        return ITER_THROW_EINVAL;

    int rc;

    if ((rc = reserve_region(v, i, count, size)))
        return rc;

    v->len += count * size;
    for (size_t j = 0; j < count; j++)
        memcpy(VEC_OFFSET(v, (i + j) * size), items[j], size);
    return ITER_OK;
}

int vec_remove(vec_t *v, void *out, size_t i, size_t count, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;
    if (vec__mul(i, size) || vec__mul(count, size))
        return ITER_THROW_EOVERFLOW;

    size_t off = i * size;
    size_t len = count * size;

    if (off + len > v->len)
        return ITER_THROW_EINVAL;

    if (out)
        memcpy(out, VEC_OFFSET(v, off), len);

    if (off + len < v->len) {
        memmove(
            VEC_OFFSET(v, off), VEC_OFFSET(v, off + len), v->len - off - len
        );
    }

    v->len -= len;
    return ITER_OK;
}

int vec_pop(vec_t *v, void *out, size_t count, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;
    return vec_remove(v, out, v->len / size, count, size);
}

static int region_overlap(size_t i, size_t j, size_t ilen, size_t jlen) {
    return (i >= j && i < j + jlen) || (j >= i && j < i + ilen);
}

int vec_swap(vec_t *v, size_t i, size_t j, size_t count, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;

    if (vec__mul(i, size) || vec__mul(j, size) || vec__mul(count, size))
        return ITER_THROW_EOVERFLOW;

    size_t ioff = i * size;
    size_t joff = j * size;
    size_t len = count * size;

    if (ioff + len > v->len || joff + len > v->len)
        return ITER_THROW_EINVAL;

    if (region_overlap(ioff, joff, len, len))
        return ITER_THROW_EINVAL;

    if (vec_reserve(v, count, size))
        return ITER_THROW_ENOMEM;

    memcpy(VEC_OFFSET(v, v->len), VEC_OFFSET(v, ioff), len);
    memcpy(VEC_OFFSET(v, ioff), VEC_OFFSET(v, joff), len);
    memcpy(VEC_OFFSET(v, joff), VEC_OFFSET(v, v->len), len);
    return ITER_OK;
}

int vec_swap_remove(vec_t *v, void *out, size_t i, size_t count, size_t size) {
    if (!v || size == 0)
        return ITER_THROW_EINVAL;

    if (vec__mul(i, size) || vec__mul(count, size))
        return ITER_THROW_EOVERFLOW;

    size_t off = i * size;
    size_t len = count * size;

    if (off + len > v->len)
        return ITER_THROW_EINVAL;

    if (off + len < v->len) {
        memmove(VEC_OFFSET(v, off), VEC_OFFSET(v, v->len - len), len);
    }

    v->len -= len;
    return ITER_OK;
}
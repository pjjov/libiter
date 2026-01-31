/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <allocator.h>
#include <iter/error.h>
#include <iter/iter.h>
#include <pf_macro.h>
#include <string.h>

#undef ITER_API
#define ITER_API
#include <iter/vector.h>

extern allocator_t *libiter_allocator;

#define VECTOR_GROWTH(old, req) ((old + req) * 1.5)

#define SORT_BUFFER_SIZE 4096

vector_t *vector__init(vector_t *vec, allocator_t *allocator) {
    if (!allocator)
        allocator = libiter_allocator;

    if (vec) {
        vec->length = vec->capacity = 0;
        vec->items = NULL;
        vec->allocator = allocator;
    }

    return vec;
}

vector_t *vector__create(allocator_t *allocator) {
    if (!allocator)
        allocator = libiter_allocator;

    vector_t *vec = allocate(allocator, sizeof(vector_t));
    return vector__init(vec, allocator);
}

vector_t *vector__with_capacity(size_t cap, allocator_t *allocator) {
    vector_t *out = vector__create(allocator);

    if (out && cap > 0 && vector__reserve(out, cap)) {
        vector__destroy(out);
        return NULL;
    }

    return out;
}

vector_t *vector__from_array(
    const void *items, size_t length, allocator_t *allocator
) {
    if (!items || length == 0)
        return NULL;

    vector_t *out = vector__with_capacity(length, allocator);

    if (out && vector__insert(out, items, 0, length)) {
        vector__destroy(out);
        return NULL;
    }

    return out;
}

void vector__destroy(vector_t *vec) {
    if (vec) {
        if (vec->allocator)
            deallocate(vec->allocator, vec->items, vec->capacity);

        vector__unwrap(vec);
    }
}

void *vector__unwrap(vector_t *vec) {
    if (!vec)
        return NULL;

    void *items = vec->items;
    deallocate(
        vec->allocator ? vec->allocator : libiter_allocator,
        vec,
        sizeof(vector_t)
    );

    return items;
}

vector_t *vector__wrap(void *items, size_t length, allocator_t *allocator) {
    if (!items || length == 0)
        return NULL;

    vector_t *vec = vector__create(allocator);
    if (vec) {
        vec->allocator = allocator;
        vec->items = items;
        vec->length = length;
        vec->capacity = length;
    }

    return vec;
}

int vector__resize(vector_t *vec, size_t capacity) {
    if (!vec || !vec->allocator)
        return ITER_EINVAL;

    void *items = reallocate(
        vec->allocator, vec->items, vec->capacity, capacity
    );

    if (!items && capacity > 0)
        return ITER_ENOMEM;

    vec->items = items;
    vec->capacity = capacity;
    if (vec->length > capacity)
        vec->length = capacity;

    return ITER_OK;
}

int vector__reserve(vector_t *vec, size_t size) {
    if (!vec || size == 0)
        return ITER_EINVAL;

    if (vec->length + size <= vec->capacity)
        return ITER_OK;

    return vector__resize(vec, VECTOR_GROWTH(vec->capacity, size));
}

int vector__insert(vector_t *vec, const void *items, size_t i, size_t size) {
    if (!vec || !items || size == 0 || i > vec->length)
        return ITER_EINVAL;

    if (vector__reserve(vec, size))
        return ITER_ENOMEM;

    if (i < vec->length) {
        void *dst = vector__slot(vec, i + size);
        void *src = vector__slot(vec, i);
        memcpy(dst, src, vec->length - i);
    }

    vec->length += size;
    memcpy(vector__slot(vec, i), items, size);
    return ITER_OK;
}

int vector__remove(vector_t *vec, size_t i, size_t size) {
    if (!vec || size == 0 || i + size > vec->length)
        return ITER_EINVAL;

    if (i + size < vec->length - 1) {
        memcpy(
            vector__slot(vec, i),
            vector__slot(vec, i + size),
            vec->length - i - size
        );
    }

    vec->length -= size;
    return ITER_OK;
}

int vector__swap_remove(vector_t *vec, size_t i, size_t size) {
    if (!vec || i + size > vec->length)
        return ITER_EINVAL;

    if (i + size < vec->length) {
        memcpy(
            vector__slot(vec, i), vector__slot(vec, vec->length - size), size
        );
    }

    vec->length -= size;
    return ITER_OK;
}

int vector__swap(vector_t *vec, size_t i, size_t j, size_t size) {
    if (!vec || i + size > vec->length || j + size > vec->length
        || (i >= j && i < j + size) || (j >= i && j < i + size))
        return ITER_EINVAL;

    if (vector__reserve(vec, size))
        return ITER_ENOMEM;

    memcpy(vector__slot(vec, vec->length), vector__slot(vec, i), size);
    memcpy(vector__slot(vec, i), vector__slot(vec, j), size);
    memcpy(vector__slot(vec, j), vector__slot(vec, vec->length), size);
    return ITER_OK;
}

void vector__free(vector_t *vec) {
    if (vec && vec->items) {
        deallocate(vec->allocator, vec->items, vec->capacity);
    }
}

size_t vector__index(const vector_t *vec, const void *item) {
    if (!vec || !item)
        return vec ? vec->length : 0;

    if (item < vec->items || item > vector__slot(vec, vec->length - 1))
        return vec->length;

    return (uint8_t *)item - (uint8_t *)vec->items;
}

int vector__each(vector_t *vec, vector_each_fn *each, void *user, size_t size) {
    if (!vec || !each || size == 0)
        return ITER_EINVAL;

    for (size_t i = 0; i < vec->length; i += size)
        if (each(vector__slot(vec, i), user))
            return ITER_EINTR;

    return ITER_OK;
}

int vector__filter(
    vector_t *vec, vector_each_fn *filter, void *user, size_t size
) {
    if (!vec || !filter || size == 0)
        return ITER_EINVAL;

    for (size_t i = 0; i < vec->length; i += size) {
        if (!filter(vector__slot(vec, i), user)) {
            vector__remove(vec, i, size);
            i -= size;
        }
    }

    return ITER_OK;
}

int vector__map(
    vector_t *dst,
    vector_t *src,
    vector_map_fn *map,
    void *user,
    size_t dsize,
    size_t ssize
) {
    if (!dst || !src || !map || dsize == 0 || ssize == 0)
        return ITER_EINVAL;

    size_t count = src->length / ssize;
    if (vector__reserve(dst, count * dsize))
        return ITER_ENOMEM;

    for (size_t i = 0; i < count; i++) {
        void *dslot = vector__slot(dst, i * dsize);
        void *sslot = vector__slot(src, i * ssize);
        if (map(dslot, sslot, user))
            return ITER_EINTR;
        dst->length += dsize;
    }

    return ITER_OK;
}

void *vector__find(
    vector_t *vec, const void *item, size_t size, vector_compare_fn *cmp
) {
    if (!vec || !item || size == 0)
        return NULL;

    if (!cmp)
        cmp = memcmp;

    size_t count = vec->length / size;

    for (size_t i = 0; i < count; i++) {
        void *out = vector__slot(vec, i * size);
        if (0 == cmp(item, out, size))
            return out;
    }

    return NULL;
}

iter_t *vector__iter(vector_t *vec, iter_t *out) {
    return vec ? iter__from_array(out, vec->items, vec->length) : NULL;
}

iter_t *vector__iter_ref(vector_t *vec, iter_t *out, size_t size) {
    return vec ? iter__ref_from_array(out, vec->items, vec->length, size)
               : NULL;
}

vector_t *vector__from_iter(iter_t *it, allocator_t *allocator, size_t stride) {
    if (!it || stride == 0)
        return NULL;

    vector_t *out = vector__with_capacity(stride, allocator);
    if (out) {
        while (!iter__call(it, vector__end(out), stride, 0)) {
            out->length += stride;

            if (vector__reserve(out, stride))
                break;
        }
    }

    return out;
}

struct sorter {
    void *A, *B;
    size_t count, size;
    size_t left, right, end;
    vector_compare_fn *compare;
};

static int mergesort_compare(struct sorter *s, size_t i, size_t j) {
    if (i >= s->right || j >= s->end)
        return i < s->right ? -1 : 0;

    return s->compare(
        PF_OFFSET(s->A, i * s->size), PF_OFFSET(s->A, j * s->size), s->size
    );
}

static void mergesort_copy(struct sorter *s, size_t dst, size_t src) {
    memcpy(
        PF_OFFSET(s->B, dst * s->size), PF_OFFSET(s->A, src * s->size), s->size
    );
}

static void mergesort_merge(struct sorter *s) {
    size_t i = s->left, j = s->right;

    for (size_t k = s->left; k < s->end; k++) {
        if (mergesort_compare(s, i, j) < 0) {
            mergesort_copy(s, k, i);
            i = i + 1;
        } else {
            mergesort_copy(s, k, j);
            j = j + 1;
        }
    }
}

static void *mergesort(struct sorter *s) {
    for (size_t w = 1; w < s->count; w *= 2) {
        for (size_t i = 0; i < s->count; i = i + 2 * w) {
            s->left = i;
            s->right = PF_MIN(i + w, s->count);
            s->end = PF_MIN(i + 2 * w, s->count);
            mergesort_merge(s);
        }

        PF_SWAP(s->A, s->B);
    }

    return s->A;
}

int vector__sort(vector_t *vec, vector_compare_fn *compare, size_t size) {
    if (!vec || size == 0 || !compare)
        return ITER_EINVAL;

    if (vec->length < size * 2)
        return ITER_OK;

    char buffer[SORT_BUFFER_SIZE];

    struct sorter s;
    s.A = vec->items;
    s.B = buffer;
    s.count = vec->length / size;
    s.size = size;
    s.compare = compare;

    if (vec->length > SORT_BUFFER_SIZE)
        s.B = allocate(libiter_allocator, vec->length);

    if (s.B == NULL)
        return ITER_ENOMEM;

    void *out = mergesort(&s);

    if (out != vec->items)
        memcpy(vec->items, out, vec->length);

    if (vec->length > SORT_BUFFER_SIZE)
        deallocate(libiter_allocator, s.B, vec->length);
    return ITER_OK;
}

int vector__is_sorted(vector_t *v, vector_compare_fn *compare, size_t size) {
    if (!v || size == 0 || !compare)
        return ITER_EINVAL;

    for (size_t i = size; i + size <= v->length; i += size)
        if (0 < compare(vector__slot(v, i - size), vector__slot(v, i), size))
            return ITER_FALSE;

    return ITER_TRUE;
}

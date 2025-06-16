/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#define ITER_API
#include <iter/error.h>
#include <iter/vector.h>

#include <allocator.h>
#include <string.h>

extern allocator_t *libiter_allocator;

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

void vector__free(vector_t *vec) {
    if (vec && vec->items) {
        deallocate(vec->allocator, vec->items, vec->capacity);
    }
}

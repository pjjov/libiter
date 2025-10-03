/*
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0

    Copyright 2025 Predrag Jovanović

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <iter/pool.h>

#include <allocator.h>
#include <pf_bitwise.h>
#include <string.h>

extern allocator_t *libiter_allocator;

#define BUCKET_SIZE 64

struct bucket {
    size_t count;
    size_t capacity;
    size_t empty;
    void *start;
    void *end;

    struct bucket *next;
    size_t flags[];
};

pool_t *pool__init(pool_t *out, allocator_t *alloc, size_t size, size_t align) {
    if (!alloc)
        alloc = libiter_allocator;

    align = pf_pow2ceilsize(align);

    if (out) {
        out->align = pf_pow2ceilsize(align);
        out->allocator = alloc;
        out->buffer = NULL;
        out->count = out->capacity = 0;
        out->size = size;
    }

    return out;
}

pool_t *pool__create(allocator_t *alloc, size_t size, size_t align) {
    if (!alloc)
        alloc = libiter_allocator;

    pool_t *out = allocate(alloc, sizeof(pool_t));
    return out ? pool__init(out, alloc, size, align) : NULL;
}

pool_t *pool__with_capacity(
    size_t capacity, allocator_t *alloc, size_t size, size_t align
) {
    pool_t *out = pool__create(alloc, size, align);

    if (pool__reserve(out, capacity)) {
        pool__destroy(out);
        return NULL;
    }

    return out;
}

void pool__destroy(pool_t *pool) {
    if (pool) {
        pool__free(pool);
        deallocate(pool->allocator, pool, sizeof(pool_t));
    }
}

void pool__free(pool_t *pool) {
    if (pool) {
        struct bucket *bucket, *next;

        for (bucket = pool->buffer; bucket; bucket = next) {
            size_t size = sizeof(struct bucket);
            size += sizeof(size_t) * bucket->capacity / BUCKET_SIZE;
            size += PF_ALIGN_PAD(size, pool->align);
            size += bucket->capacity * pool->size;

            next = bucket->next;
            deallocate(pool->allocator, bucket, size);
        }
    }
}

int pool__reserve(pool_t *pool, size_t count) {
    if (!pool || count == 0)
        return ITER_EINVAL;

    if (pool->count + count > pool->capacity) {
        struct bucket *bucket;
        size_t cap = PF_ALIGN_UP((pool->count + count) * 2, BUCKET_SIZE);

        size_t offset = sizeof(struct bucket);
        offset += sizeof(size_t) * cap / BUCKET_SIZE;
        offset += PF_ALIGN_PAD(offset, pool->align);

        bucket = allocate(pool->allocator, offset + cap * pool->size);
        if (!bucket)
            return ITER_ENOMEM;

        bucket->count = bucket->empty = 0;
        bucket->capacity = cap;
        bucket->start = (void *)((uintptr_t)bucket + offset);
        bucket->end = (void *)((uintptr_t)bucket->start + cap * pool->size);
        bucket->next = pool->buffer;
        memset(bucket->flags, 0, sizeof(size_t) * cap / BUCKET_SIZE);

        pool->capacity += cap;
        pool->buffer = bucket;
    }

    return ITER_OK;
}

int pool__give(pool_t *pool, void *item) {
    if (!pool || !item)
        return ITER_EINVAL;

    struct bucket *bucket = pool->buffer;
    for (; bucket; bucket = bucket->next) {
        if (bucket->start <= item && bucket->end >= item)
            break;
    }

    if (!bucket)
        return ITER_ENOENT;

    size_t index = ((uintptr_t)item - (uintptr_t)bucket->start) / pool->size;
    size_t mask = 1 << (index % BUCKET_SIZE);

    if (!(bucket->flags[index / BUCKET_SIZE] & mask))
        return ITER_ENOENT;

    bucket->flags[index / BUCKET_SIZE] &= ~mask;
    bucket->count--;
    pool->count--;

    if (bucket->empty > index)
        bucket->empty = index;
    return ITER_OK;
}

void *pool__take(pool_t *pool) {
    if (!pool || pool__reserve(pool, 1))
        return NULL;

    struct bucket *bucket = pool->buffer;
    for (; bucket; bucket = bucket->next)
        if (bucket->count < bucket->capacity)
            break;

    for (size_t i = bucket->empty; i < bucket->capacity / BUCKET_SIZE; i++) {
        if (bucket->flags[i] != ~0) {
            int bit = pf_ctzsize(bucket->flags[i]) - 1;
            bucket->flags[i] |= 1 << bit;
            bucket->count++;
            pool->count++;

            bucket->empty = i;
            size_t offset = (i * BUCKET_SIZE + bit) * pool->size;
            return (void *)((uintptr_t)bucket->start + offset);
        }
    }

    return NULL;
}

size_t pool__to_index(pool_t *pool, void *item) {
    if (!pool || !item)
        return 0;

    struct bucket *bucket = pool->buffer;
    size_t index = 0;

    for (; bucket; bucket = bucket->next) {
        if (bucket->start <= item && bucket->end >= item) {
            index += (item - bucket->start) / pool->size;
            break;
        }

        index += bucket->capacity;
    }

    return index;
}

void *pool__from_index(pool_t *pool, size_t index) {
    if (!pool || index >= pool->capacity)
        return NULL;

    struct bucket *bucket = pool->buffer;
    for (; bucket; bucket = bucket->next) {
        if (index < bucket->capacity) {
            size_t offset = index * pool->size;
            return (void *)((uintptr_t)bucket->start + offset);
        }

        index -= bucket->capacity;
    }

    return NULL;
}

static int pool_iter_fn(iter_t *it, void *out, size_t size, size_t skip) {
    if (!it || it == out)
        return ITER_EINVAL;

    pool_t *pool = it->container;
    const struct bucket *bucket = it->bucket;
    uintptr_t i = (uintptr_t)it->current;

    if (size != pool->size)
        return ITER_EINVAL;

    if (out)
        skip++;

    uintptr_t set = i / BUCKET_SIZE;
    uintptr_t bit = i % BUCKET_SIZE;

    while (bucket) {
        if (bucket->flags[set] & (1 << bit))
            skip--;

        if (skip == 0)
            break;

        if (++bit >= BUCKET_SIZE) {
            set++;
            bit = 0;
        }

        if (++i >= bucket->capacity) {
            bucket = bucket->next;
            i = 0;
        }
    }

    i = set * BUCKET_SIZE + bit;
    it->bucket = bucket;
    it->current = (void *)i;

    if (!bucket)
        return ITER_ENODATA;

    uintptr_t value = (uintptr_t)bucket->start + pool->size * i;
    memcpy(out, (void *)value, pool->size);
    return ITER_OK;
}

iter_t *pool__iter(pool_t *pool, iter_t *out) {
    if (!out || !pool)
        return NULL;

    out->call = &pool_iter_fn;
    out->container = pool;
    out->bucket = pool->buffer;
    out->current = (void *)(uintptr_t)0;
    return out;
}

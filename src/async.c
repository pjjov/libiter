/*  libiter - Generic container and iterator library for C.

    Copyright 2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/async.h>
#include <iter/error.h>

#include <allocator.h>

#define ASYNC_MAX_THREADS 32

struct executor_t {
    allocator_t *allocator;
    size_t threadCount;
};

extern allocator_t *libiter_allocator;

static struct executor_t default_executor = { 0 };
executor_t *libiter_executor = &default_executor;

struct executor_opt *fix_options(struct executor_opt *opt) {
    if (!opt->allocator)
        opt->allocator = libiter_allocator;
    if (opt->threadCount > ASYNC_MAX_THREADS)
        opt->threadCount = ASYNC_MAX_THREADS;
    return opt;
}

executor_t *executor_create(struct executor_opt *opt) {
    struct executor_opt _options = { 0 };
    opt = fix_options(opt ? opt : &_options);

    executor_t *exec;

    if (!(exec = allocate(opt->allocator, sizeof(*exec))))
        return NULL;

    exec->allocator = opt->allocator;
    exec->threadCount = opt->threadCount;
    return exec;
}

void executor_destroy(executor_t *exec) {
    if (!exec)
        return;

    if (exec->allocator)
        deallocate(exec->allocator, exec, sizeof(*exec));
}

future_t *executor__run(
    async_fn *fn,
    const void *args,
    executor_t *exec,
    size_t futureSize,
    size_t promiseSize
) {
    if (!exec)
        exec = libiter_executor;

    return NULL;
}

int future__poll(future_t *fut, void *out, int timeout, size_t size) {
    if (!fut || !out)
        return ITER_EINVAL;

    return ITER_ENOSYS;
}

int future__await(future_t *fut, void *out, size_t size) {
    if (!fut || !out)
        return ITER_EINVAL;

    return ITER_ENOSYS;
}

int future__cancel(future_t *fut) {
    if (!fut)
        return ITER_EINVAL;

    return ITER_ENOSYS;
}

int promise__await(promise_t *p, future_t *fut, void *out, int *status) {
    if (!p || !fut || !out || !status)
        return ITER_EINVAL;

    return ITER_ENOSYS;
}

int promise_is_canceled(promise_t *p) { return ITER_FALSE; }

void *promise_data(promise_t *p) { return NULL; }

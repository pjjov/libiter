/*  libiter - Generic container and iterator library for C.

    Copyright 2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#include <iter/async.h>
#include <iter/error.h>

#include <allocator.h>
#include <string.h>
#include <threads.h>

#define ASYNC_MAX_THREADS 32

struct promise_t {
    executor_t *exec;
    async_fn *fn;

    size_t promiseSize;
    size_t futureSize;

    promise_t *next;
    future_t *future;
    char data[];
};

struct executor_t {
    allocator_t *allocator;
    size_t threadCount;
    mtx_t lock;

    cnd_t queueCond;
    promise_t *queueHead;
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

static inline int executor_lock(executor_t *exec) {
    return thrd_success == mtx_lock(&exec->lock) ? ITER_OK : ITER_ENOLCK;
}

static inline int executor_unlock(executor_t *exec) {
    return thrd_success == mtx_unlock(&exec->lock) ? ITER_OK : ITER_ENOLCK;
}

executor_t *executor_create(struct executor_opt *opt) {
    struct executor_opt _options = { 0 };
    opt = fix_options(opt ? opt : &_options);

    executor_t *exec;

    if (!(exec = allocate(opt->allocator, sizeof(*exec))))
        return NULL;

    exec->allocator = opt->allocator;
    exec->threadCount = opt->threadCount;
    exec->queueHead = NULL;
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

    if (executor_lock(exec))
        return NULL;

    promise_t *promise = allocate(
        exec->allocator, sizeof(promise_t) + promiseSize
    );
    future_t *future = allocate(exec->allocator, sizeof(future_t) + futureSize);

    if (!future || !promise) {
        deallocate(exec->allocator, promise, sizeof(promise_t) + promiseSize);
        deallocate(exec->allocator, future, sizeof(future_t) + futureSize);
        executor_unlock(exec);
        return NULL;
    }

    promise->exec = exec;
    promise->fn = fn;
    promise->future = future;
    promise->promiseSize = promiseSize;
    promise->futureSize = futureSize;
    memcpy(promise->data, args, promiseSize);

    future->exec = exec;
    memset(future->item, 0, futureSize);

    promise->next = exec->queueHead;
    exec->queueHead = promise;
    executor_unlock(exec);
    cnd_signal(&exec->queueCond);
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

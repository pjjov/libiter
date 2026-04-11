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

struct future_t {
    executor_t *exec;
    promise_t *promise;
    char item[];
};

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
    int terminate;
    mtx_t lock;

    cnd_t doneCond;
    cnd_t readyCond;
    promise_t *readyHead;

    thrd_t threads[ASYNC_MAX_THREADS];
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

static int run_promise(executor_t *exec, promise_t *promise) {
    executor_unlock(exec);

    future_t *future = promise->future;
    int status = promise->fn(promise, future);
    (void)status;

    executor_lock(exec);

    future->promise = NULL;
    deallocate(
        exec->allocator, promise, sizeof(promise_t) + promise->promiseSize
    );

    return ITER_OK;
}

static int executor_thread_fn(void *user) {
    executor_t *exec = user;

    if (executor_lock(exec))
        return ITER_ENOLCK;

    while (!exec->terminate) {
        if (exec->readyHead == NULL) {
            cnd_wait(&exec->readyCond, &exec->lock);
            continue;
        }

        promise_t *task = exec->readyHead;
        exec->readyHead = task->next;
        run_promise(exec, task);
    }

    executor_unlock(exec);
    return ITER_OK;
}

static inline void start_threads(executor_t *exec, size_t count) {
    size_t started = 0;
    for (size_t i = 0; i < count; i++)
        if (thrd_create(&exec->threads[started], executor_thread_fn, exec))
            started++;

    exec->threadCount = started;
}

static inline int init_locks(executor_t *exec) {
    int mtx = (mtx_init(&exec->lock, mtx_timed) == thrd_success);
    int rcnd = (cnd_init(&exec->readyCond) == thrd_success);
    int dcnd = (cnd_init(&exec->doneCond) == thrd_success);

    if (!mtx || !rcnd || !dcnd) {
        if (!mtx)
            mtx_destroy(&exec->lock);
        if (!rcnd)
            cnd_destroy(&exec->readyCond);
        if (!dcnd)
            cnd_destroy(&exec->doneCond);
        return ITER_EASYNC;
    }

    return ITER_OK;
}

executor_t *executor_create(struct executor_opt *opt) {
    struct executor_opt _options = { 0 };
    opt = fix_options(opt ? opt : &_options);

    executor_t *exec;

    if (!(exec = allocate(opt->allocator, sizeof(*exec))))
        return NULL;

    exec->allocator = opt->allocator;
    exec->threadCount = 0;
    exec->readyHead = NULL;

    if (init_locks(exec)) {
        deallocate(opt->allocator, exec, sizeof(*exec));
        return NULL;
    }

    start_threads(exec, opt->threadCount);
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

    future->promise = promise;
    memset(future->item, 0, futureSize);

    promise->next = exec->readyHead;
    exec->readyHead = promise;
    executor_unlock(exec);
    cnd_signal(&exec->readyCond);
    return NULL;
}

int future__poll(future_t *fut, void *out, int timeout, size_t size) {
    if (!fut || !out)
        return ITER_EINVAL;

    if (timeout != 0)
        return ITER_ENOSYS;

    if (executor_lock(fut->exec))
        return ITER_ENOLCK;

    if (fut->promise == NULL) {
        memcpy(out, fut->item, size);
        deallocate(fut->exec->allocator, fut, sizeof(*fut) + size);
        executor_unlock(fut->exec);
        return ITER_OK;
    }

    executor_unlock(fut->exec);
    return ITER_ETIMEDOUT;
}

int future__await(future_t *fut, void *out, size_t size) {
    if (!fut || !out)
        return ITER_EINVAL;

    return ITER_ENOSYS;
}

int future__handle(future_t *fut, await_fn *handler, void *user) {
    return ITER_ENOSYS;
}

int promise__await(promise_t *p, future_t *fut, void *out, int *status) {
    if (!p || !fut || !out || !status)
        return ITER_EINVAL;

    return ITER_ENOSYS;
}

void *promise_data(promise_t *p) { return p ? p->data : NULL; }

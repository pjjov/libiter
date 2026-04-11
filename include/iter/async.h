/*  libiter - Generic container and iterator library for C.

    Copyright 2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef ITER_ASYNC_H
#define ITER_ASYNC_H

#define LIBITER_NEED_TYPE
#define LIBITER_NEED_ASYNC
#include <iter/types.h>

#ifndef ITER_API
    #define ITER_API
#endif

#ifndef ITER_INLINE
    #define ITER_INLINE static inline
#endif

#define future_type(m_fut) generic_value_type(future_t, m_fut)
#define future_type_ptr(m_fut) generic_value_ptr(future_t, m_fut)
#define future_type_size(m_fut) generic_value_size(future_t, m_fut)
#define future_as_base(m_fut) generic_check_container(future_t, void, m_fut)
#define future_check_type(m_fut, m_item)         \
    generic_check_value(future_t, m_fut, m_item)

struct executor_opt {
    allocator_t *allocator;
    size_t threadCount;
};

/** Creates a new executor object using provided `options`. **/
ITER_API executor_t *executor_create(struct executor_opt *options);

/** Executes a single task using the calling thread.
    Possible error codes: ITER_EINVAL, ITER_ENODATA.
**/
ITER_API int executor_poll(executor_t *exec);

/** Destroys the `exec` and it's associated objects. **/
ITER_API void executor_destroy(executor_t *exec);

/** nanodoc.inline-decl on **/

/** future(F) executor_run(type F, async_fn *fn, P *args, executor_t *exec);

    Creates a new future-promise pair that will be executed by `exec`.
    The data from `args` will be copied to a promise object and made
    available to the asynchronous `fn`.
**/
#define executor_run(F, m_fn, m_args, m_exec)                              \
    ((future(F))                                                           \
         executor__run(m_fn, m_args, m_exe, sizeof(F), sizeof(*(m_args))))

ITER_API future_t *executor__run(
    async_fn *fn,
    const void *args,
    executor_t *exec,
    size_t futureSize,
    size_t promiseSize
);

/** future(F) future_run(type F, async_fn *fn, P *args);

    Creates a new future-promise pair that will be executed by the default
    executor. The data from `args` will be copied to a promise object and
    made available to the asynchronous `fn`.
**/
#define future_run(F, m_fn, m_args)                                      \
    ((future(F))future__run(m_fn, m_args, sizeof(F), sizeof(*(m_args))))

ITER_INLINE future_t *future__run(
    async_fn *fn, const void *args, size_t futureSize, size_t promiseSize
) {
    return executor__run(fn, args, NULL, futureSize, promiseSize);
}

/** int future_poll(future(T) fut, T *out, int timeout);

    Checks if `fut` is complete and, if it is, moves the resulting
    data to `out` and frees the resources of the `fut` object.

    Parameter `timeout` specifies the number of milliseconds to wait
    for completion of `fut`. If `timeout` is negative, the function
    is equivalent to `future_await`.

    Possible error codes: ITER_EINVAL, ITER_ETIMEDOUT.
**/
#define future_poll(m_fut, m_out, m_timeout) \
    future__poll(                            \
        future_as_base(m_fut),               \
        future_check_type(m_fut, m_out),     \
        (m_timeout),                         \
        future_type_size(m_fut)              \
    )

ITER_API int future__poll(future_t *fut, void *out, int timeout, size_t size);

/** int future_await(future(T) fut, T *out);

    Waits until `fut` is complete and, if it is, moves the resulting
    data to `out` and frees the resources of the `fut` object.

    Possible error codes: ITER_EINVAL.
**/
#define future_await(m_fut, m_out)       \
    future__poll(                        \
        future_as_base(m_fut),           \
        future_check_type(m_fut, m_out), \
        0,                               \
        future_type_size(m_fut)          \
    )

/** int future_handle(future(T) fut, await_fn *handler, void *user);

    Sets the handler for awaiting the result of `fut`, which will be
    called from the same thread that executed the async function.

    > To free future's resources, a call to `future_poll` is still required.

    Possible error codes: ITER_EINVAL.
**/
#define future_handle(m_fut, m_handler, m_user)                  \
    future__handle(future_as_base(m_fut), (m_handler), (m_user))

ITER_API int future__handle(future_t *fut, await_fn *handler, void *user);

/** int promise_await(promise_t *p, future(T) fut, T *out, int *status);

    Tells the executor to resume the execution of the promise after `fut`
    is completed. The resulting data will be copied to `out` and `status`
    will be filled with the status of the awaited `fut`.

    > The asynchronous function must exit to await the future.

    Possible error codes: ITER_EINVAL.
**/
#define promise_await(m_p, m_fut, m_out, m_status)                    \
    promise__await((m_p), future_as_base(m_fut), (m_out), (m_status))

ITER_API int promise__await(
    promise_t *p, future_t *fut, void *out, int *status
);

/** void *promise_data(promise_t *p);

    Returns the copy of the arguments given while creating the promise.
    This pointer will remain valid and available across multiple calls of
    the corresponding asynchronous function and should be used to store
    the user state of the promise.
**/
ITER_API void *promise_data(promise_t *p);

#endif

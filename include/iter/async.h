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

    Possible error codes: ITER_EINVAL, ITER_ETIMEDOUT, ITER_ECANCELED.
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

    Possible error codes: ITER_EINVAL, ITER_ECANCELED.
**/
#define future_await(m_fut, m_out)       \
    future__await(                       \
        future_as_base(m_fut),           \
        future_check_type(m_fut, m_out), \
        future_type_size(m_fut)          \
    )

ITER_API int future__await(future_t *fut, void *out, size_t size);

/** int future_cancel(future(T) fut);

    Cancels the delivery of `fut` object, but not necessarily it's execution.
    The cleanup of resources should be handled by it's asynchronous function.

    Possible error codes: ITER_EINVAL, ITER_ECANCELED.
**/
#define future_cancel(m_fut) future__cancel(future_as_base(m_fut))

ITER_API int future__cancel(future_t *fut);

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

/** int promise_is_canceled(promise_t *p);

    Checks if the corresponding future is canceled.
**/
ITER_API int promise_is_canceled(promise_t *p);

/** void *promise_data(promise_t *p);

    Returns the copy of the arguments given while creating the promise.
    This pointer will remain valid and available across multiple calls of
    the corresponding asynchronous function and should be used to store
    the user state of the promise.
**/
ITER_API void *promise_data(promise_t *p);

#endif

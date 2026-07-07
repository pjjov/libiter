/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_GLOBAL_H
#define LIBITER_GLOBAL_H

#ifndef ITER_API
    #define ITER_API
#endif

#ifndef ITER_INLINE
    #define ITER_INLINE static inline
#endif

#ifndef ITER_NO_RETURN
    #define ITER_NO_RETURN
#endif

#define LIBITER_EXCEPTION (('l' << 8) | 'i')

#include <stdarg.h>

typedef struct allocator_t allocator_t;
typedef struct pf_exception_t pf_exception_t;

/** Function: libiter_use_allocator

    This function sets the default allocator of the library.
*/
ITER_API void libiter_use_allocator(allocator_t *allocator);

/** Function: libiter_catch

    This function set's up the exception handler for the `<pf_exception.h>`
    exception handling interface. The first call will return `NULL`, after
    which exception-throwing code can be executed.

    The second return value will represent the emitted exception.

    > The global exception stack is thread local.
*/
ITER_API pf_exception_t *libiter_catch(int error);

#define ITER_THROW(code, msg) libiter_throw((code), __func__, (msg))
#define ITER_THROWF(code, fmt, ...)                      \
    libiter_throwf((code), __func__, (fmt), __VA_ARGS__)
#define ITER_VTHROWF(code, fmt, args)                \
    libiter_vthrowf((code), __func__, (fmt), (args))

#define ITER_THROW_NULL(code) (ITER_THROW(code, NULL), NULL)

#define ITER_THROW_ENOENT ITER_THROW(ITER_ENOENT, NULL)
#define ITER_THROW_EINTR ITER_THROW(ITER_EINTR, NULL)
#define ITER_THROW_EAGAIN ITER_THROW(ITER_EAGAIN, NULL)
#define ITER_THROW_ENOMEM ITER_THROW(ITER_ENOMEM, NULL)
#define ITER_THROW_EEXIST ITER_THROW(ITER_EEXIST, NULL)
#define ITER_THROW_EINVAL ITER_THROW(ITER_EINVAL, NULL)
#define ITER_THROW_ENOLCK ITER_THROW(ITER_ENOLCK, NULL)
#define ITER_THROW_ENOSYS ITER_THROW(ITER_ENOSYS, NULL)
#define ITER_THROW_ENODATA ITER_THROW(ITER_ENODATA, NULL)
#define ITER_THROW_EOVERFLOW ITER_THROW(ITER_EOVERFLOW, NULL)
#define ITER_THROW_ETIMEDOUT ITER_THROW(ITER_ETIMEDOUT, NULL)
#define ITER_THROW_ECANCELED ITER_THROW(ITER_ECANCELED, NULL)
#define ITER_THROW_EASYNC ITER_THROW(ITER_EASYNC, NULL)

/** Function: libiter_throw
    Emits an exception with given message.
*/
ITER_NO_RETURN ITER_API int libiter_throw(
    int code, const char *func, const char *msg
);

/** Function: libiter_throwf
    Emits an exception with given formatting.
*/
ITER_NO_RETURN ITER_API int libiter_throwf(
    int code, const char *func, const char *fmt, ...
);

/** Function: libiter_vthrowf
    Emits an exception with given formatting.
*/
ITER_NO_RETURN ITER_API int libiter_vthrowf(
    int code, const char *func, const char *fmt, va_list args
);

/** Function: libiter_rethrow
    Emits the exception `e` for the next catcher.
*/
ITER_NO_RETURN ITER_API int libiter_rethrow(pf_exception_t *e);

#endif
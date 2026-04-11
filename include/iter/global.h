/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_GLOBAL_H
#define LIBITER_GLOBAL_H

#include <allocator.h>
#include <iter/hash.h>

#ifndef ITER_API
    #define ITER_API
#endif

#ifndef ITER_INLINE
    #define ITER_INLINE static inline
#endif

typedef struct executor_t executor_t;

/** # Default allocator

    This function sets the default allocator for containers and returns the
    previous one. Containers created before calling this function will continue
    to use the previously set allocator. If `NULL` is passed, the original
    allocator will be used again.

    **This function is NOT thread-safe.**
**/
ITER_API allocator_t *libiter_use_allocator(allocator_t *allocator);

/** # Default hasher

    This function sets the default hasher for containers and returns the
    previous one. Containers created before calling this function will continue
    to use the previously set hasher. If `NULL` is passed, the original
    hasher will be used again.

    **This function is NOT thread-safe.**
**/
ITER_API hasher_fn *libiter_use_hasher(hasher_fn *hasher);

/** # Default hasher

    This function sets the default executor for asynchronous functions and
    returns the previous one. Containers created before calling this function
    will continue to use the previously set executor.

    Unlike other default objects, the default executor must be explicitly
    created and destroyed by the user in order to use asynchronous functions.

    ```c
    int main() {
        executor_t *exec = executor_create(NULL);
        libiter_use_executor(exec);

        // your application code

        executor_join(exec);
        libiter_use_executor(NULL);

        // finish remaining tasks
        for (int result = ITER_OK; !result;)
            result = executor_poll(exec);

        executor_destroy(exec);
        return 0;
    }
    ```

    **This function is NOT thread-safe.**
**/
ITER_API executor_t *libiter_use_executor(executor_t *executor);

#endif

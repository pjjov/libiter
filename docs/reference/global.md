<!--
    libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
-->

# Global state management and exception handling.

- [`libiter_use_allocator`](#libiter_use_allocator) — This function sets the default allocator of the library.
- [`libiter_catch`](#libiter_catch) — This function set's up the exception handler for the `<pf_exception.h>` exception handling interface.
- [`libiter_throw`](#libiter_throw) — Emits an exception with given message.
- [`libiter_throwf`](#libiter_throwf) — Emits an exception with given formatting.
- [`libiter_vthrowf`](#libiter_vthrowf) — Emits an exception with given formatting.
- [`libiter_rethrow`](#libiter_rethrow) — Emits the exception `e` for the next catcher.

<a id="libiter_use_allocator"></a>

## `libiter_use_allocator`

```c
ITER_API void libiter_use_allocator(allocator_t *allocator);
```

This function sets the default allocator of the library.

<a id="libiter_catch"></a>

## `libiter_catch`

```c
ITER_API pf_exception_t *libiter_catch(int error);
```

This function set's up the exception handler for the `<pf_exception.h>`
exception handling interface. The first call will return `NULL`, after
which exception-throwing code can be executed.

The second return value will represent the emitted exception.

> The global exception stack is thread local.

<a id="libiter_throw"></a>

## `libiter_throw`

```c
ITER_NO_RETURN ITER_API int libiter_throw(
    int code, const char *func, const char *msg
);
```

Emits an exception with given message.

<a id="libiter_throwf"></a>

## `libiter_throwf`

```c
ITER_NO_RETURN ITER_API int libiter_throwf(
    int code, const char *func, const char *fmt, ...
);
```

Emits an exception with given formatting.

<a id="libiter_vthrowf"></a>

## `libiter_vthrowf`

```c
ITER_NO_RETURN ITER_API int libiter_vthrowf(
    int code, const char *func, const char *fmt, va_list args
);
```

Emits an exception with given formatting.

<a id="libiter_rethrow"></a>

## `libiter_rethrow`

```c
ITER_NO_RETURN ITER_API int libiter_rethrow(pf_exception_t *e);
```

Emits the exception `e` for the next catcher.


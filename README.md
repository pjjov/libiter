<!--
    libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
-->

# libiter

**libiter** is a generic container and iterator library for C.

Using modern techniques, **libiter** represents a simple and optimized
framework for manipulating data, while minimizing the usage of macros
and non-standard features.

```c

vec(int) ints = vecnew(int, NULL);
map(int) mappings = mapnew(int, hash_int, NULL);
int values[] = { 1, 2, 3 };

vecpush(ints, values, 3);

vecshift(v, NULL, 1);

mapset(mappings, vecptr(v, 2));

VECFOREACH(v, var) {
    printf("%d %p\n", var, mapptr(mappings, &var));
}

vecfree(ints);
mapfree(mappings);

```

**libiter** consists of:

- `vec(T)` - growable array like `std::vector` from C++.
- `map(T)` - associative container that stores items by hashing.
- `generic.h` - utilities for implementing generic types.

## Documentation

Reference documentation for each container and their functions are found
in corresponding header files inside Javadoc-like comments (`/** ... */`).

Because generic types are implemented through a layer of macros, the documentation
has been written with *pseudo-C* style declarations that mimics the C++ template syntax.

## Building

Through the [cpolyfill](github.com/pjjov/cpolyfill) library,
**libiter** maintains C99 compatibility.

### Meson

```sh
meson setup build
cd build
meson compile
meson test # optional
```

```meson
libiter_dep = dependency('iter', 'libiter', fallback : 'libiter')
```

## License

See the [LICENSE](./LICENSE) file for more information.

<!--
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
-->

# libiter - generic containers and iterators for C.

**libiter** is a generic container and iterator library for C.

Using modern techniques, **libiter** represents a simple and optimized
framework for manipulating data, while minimizing the usage of macros
and non-standard features.

```c
#include <iter/vector.h>
#include <iter/hashmap.h>
#include <math.h>

hashmap(int, double) example(int *buffer, size_t size) {

    vector(int) ints = vector_wrap(buffer, size, NULL);
    vector(double) roots = vector_with_capacity(double, size, NULL);

    for (size_t i = 0; i < vector_length(ints); i++) {
        double root = sqrt(*vector_get(ints, i));
        vector_push(roots, &root, 1);
    }

    hashmap(int, double) map = hashmap_from_vectors(
        vector_items(ints),
        vector_items(roots),
        vector_length(ints),
        NULL
    );

    return map;
}
```

**libiter** consists of:

- `vector(T)`     - growable array like `std::vector` from C++.
- `hashmap(K, V)` - associative container storing key-value pairs.
- `iter(T)`       - generic iterator interface.
- `pool(T)`       - object pool with fast insertion and deletion operations.
- `generic.h`     - utilities for implementing generic types.

## Documentation

Reference documentation for each container and their functions are found
in corresponding header files inside Javadoc-like comments (`/** ... **/`).

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

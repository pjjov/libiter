# Copyright 2025-2026 Предраг Јовановић
# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

allocator_t_dep = dependency('allocator_t', required: true)
cpolyfill_dep = dependency('cpolyfill', required: true)

deps = [allocator_t_dep, cpolyfill_dep]

inc = include_directories('../include')

lib = library(
    'iter',
    dependencies: deps,
    include_directories: inc,
    install: true,
    sources: [
        'global.c',
        'map.c',
        'vec.c',
    ]
)

iter_dep = declare_dependency(
    dependencies: deps,
    include_directories: inc,
    link_with: [lib]
)

meson.override_dependency('iter', iter_dep)
meson.override_dependency('libiter', iter_dep)
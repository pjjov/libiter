# Copyright 2025-2026 Предраг Јовановић
# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

project('libiter', 'c', version: '2.0.1')

src = [
    'src/global.c',
    'src/map.c',
    'src/vec.c',
]

inc = include_directories('include')

allocator_t_dep = dependency('allocator_t', required: true)
cpolyfill_dep = dependency('cpolyfill', required: true)

deps = [allocator_t_dep, cpolyfill_dep]

lib = library(
    'iter',
    dependencies: deps,
    include_directories: inc,
    install: true,
    sources: src
)

iter_dep = declare_dependency(
    dependencies: deps,
    include_directories: inc,
    link_with: [lib]
)

meson.override_dependency('iter', iter_dep)
meson.override_dependency('libiter', iter_dep)

install_headers(
    'include/iter/global.h',
    'include/iter/map.h',
    'include/iter/pf_types.h',
    'include/iter/types.h',
    'include/iter/vec.h',
    subdir: 'iter'
)


pkg = import('pkgconfig')
pkg.generate(
    lib,
    name: 'libiter',
    filebase: 'libiter',
    description: 'Generic container and iterator library for C.',
    version: meson.project_version(),
    requires: [
        'allocator_t',
        'cpolyfill',
    ],
    extra_cflags: '-I${includedir}',
)

if not meson.is_subproject()
    test_exe = executable(
        'libiter-test',
        sources: [
            'test/main.c',
            'test/vec.c',
            'test/map.c',
        ],
        dependencies: [cpolyfill_dep, iter_dep],
    )

    test('libiter/vec', test_exe, args: ['vec'])
    test('libiter/map', test_exe, args: ['map'])
endif

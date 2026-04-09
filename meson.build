# Copyright 2025 Predrag Jovanović
# SPDX-FileCopyrightText: 2025 Predrag Jovanović
# SPDX-License-Identifier: Apache-2.0

project('libiter', 'c')

src = [
    'src/async.c',
    'src/bitmap.c',
    'src/global.c',
    'src/hashmap.c',
    'src/iter.c',
    'src/pool.c',
    'src/vector.c',
]

inc = include_directories('include')

deps = [
    dependency('allocator_t', required: true),
    dependency('cpolyfill', required: true),
]

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
    'include/iter/async.h',
    'include/iter/bitmap.h',
    'include/iter/error.h',
    'include/iter/generic.h',
    'include/iter/global.h',
    'include/iter/hash.h',
    'include/iter/hashmap.h',
    'include/iter/iter.h',
    'include/iter/pool.h',
    'include/iter/vector.h',
    subdir: 'iter'
)

if not meson.is_subproject()
    tests = executable(
        'libiter-test',
        dependencies: [iter_dep],
        install: false,
        sources: [
            'test/bitmap.c',
            'test/hashmap.c',
            'test/iter.c',
            'test/main.c',
            'test/pool.c',
            'test/vector.c',
        ]
    )

    test('libiter/bitmap', tests, args: ['bitmap'], protocol: 'tap')
    test('libiter/hashmap', tests, args: ['hashmap'], protocol: 'tap')
    test('libiter/iter', tests, args: ['iter'], protocol: 'tap')
    test('libiter/pool', tests, args: ['pool'], protocol: 'tap')
    test('libiter/vector', tests, args: ['vector'], protocol: 'tap')
endif

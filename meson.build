# Copyright 2025 Predrag Jovanović
# SPDX-FileCopyrightText: 2025 Predrag Jovanović
# SPDX-License-Identifier: Apache-2.0

project('libiter', 'c')

src = [
    'src/global.c',
    'src/hashmap.c',
    'src/iter.c',
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
    sources: src
)

iter_dep = declare_dependency(
    dependencies: deps,
    include_directories: inc,
    link_with: [lib]
)

meson.override_dependency('iter', iter_dep)
meson.override_dependency('libiter', iter_dep)

tests = executable(
    'libiter-test',
    dependencies: [iter_dep],
    sources: [
        'test/hashmap.c',
        'test/iter.c',
        'test/main.c',
        'test/vector.c',
    ]
)

test('libiter/hashmap', tests, args: ['hashmap'], protocol: 'tap')
test('libiter/iter', tests, args: ['iter'], protocol: 'tap')
test('libiter/vector', tests, args: ['vector'], protocol: 'tap')

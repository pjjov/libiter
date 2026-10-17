# Copyright 2025-2026 Предраг Јовановић
# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

project('libiter', 'c', version: '2.0.1')

subdir('src')

install_headers(
    'iter/global.h',
    'iter/map.h',
    'iter/pf_types.h',
    'iter/types.h',
    'iter/vec.h',
    subdir: 'iter'
)

install_man(
    'docs/iter-global.3',
    'docs/iter-map.3',
    'docs/iter-types.3',
    'docs/iter-vec.3',
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
    subdir('test')
endif

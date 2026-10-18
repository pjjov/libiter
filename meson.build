# Copyright 2025-2026 Предраг Јовановић
# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

project('libiter', 'c', version: '2.0.2')

subdir('src')

install_headers(
    'include/iter/global.h',
    'include/iter/map.h',
    'include/iter/pf_types.h',
    'include/iter/types.h',
    'include/iter/vec.h',
    subdir: 'iter'
)

install_man(
    'docs/manual/iter-global.3',
    'docs/manual/iter-map.3',
    'docs/manual/iter-types.3',
    'docs/manual/iter-vec.3',
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

# Copyright 2025-2026 Предраг Јовановић
# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

test_exe = executable(
    'libiter-test',
    sources: [
        'main.c',
        'vec.c',
        'map.c',
    ],
    dependencies: [cpolyfill_dep, iter_dep],
)

test('libiter/vec', test_exe, args: ['vec'])
test('libiter/map', test_exe, args: ['map'])

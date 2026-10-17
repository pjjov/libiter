#!/bin/sh
# Copyright 2025-2026 Предраг Јовановић
# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0
#
# Regenerates docs/manual (man pages) and docs/reference (Markdown) from the
# public headers using nanodoc. Run from anywhere.

set -e

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"

version=$(sed -n "s/^project(.*version: *'\([^']*\)').*/\1/p" meson.build)
headers="include/iter/global.h include/iter/map.h include/iter/types.h include/iter/vec.h"

rm -rf docs/manual docs/reference
mkdir -p docs/manual docs/reference

# shellcheck disable=SC2086
nanodoc -k docs/nanodoc.txt -f md -s -t libiter --header docs/header.md \
    -o docs/reference $headers

# Manual pages are generated per module (one file per header).
for h in $headers; do
    name=$(basename "$h" .h)
    nanodoc -k docs/nanodoc.txt -f man --synopsis \
        --man-source "libiter $version" --man-manual 'libiter manual' \
        --header docs/header.man --footer docs/footer.man \
        -o "docs/manual/iter-$name.3" "$h"
done

# nanodoc names headers by basename; the public include path is iter/<name>.
sed -i 's|#include <\([a-z_]*\.h\)>|#include <iter/\1>|' docs/manual/*.3

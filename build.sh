#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT="$HERE/build"
mkdir -p "$OUT"

CC=$(command -v clang || true)
[ -n "$CC" ] || { echo "Xcode command line tools are required." >&2; exit 1; }

"$CC" -arch arm64 -arch x86_64 -O2 \
    -o "$OUT/foo2zjs" \
    "$HERE/src/foo2zjs.c" "$HERE/src/jbig.c" "$HERE/src/jbig_ar.c"

"$CC" -arch arm64 -arch x86_64 -O2 -Wall -Wextra \
    -o "$OUT/rastertozjs" "$HERE/filter/rastertozjs.c" -lcups

echo "Built:"
file "$OUT/foo2zjs" "$OUT/rastertozjs"

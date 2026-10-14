#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d /private/tmp/p1566-smoke.XXXXXX)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

"$HERE/build.sh" >/dev/null
cp "$HERE/build/foo2zjs" "$TMP/foo2zjs"
clang -arch arm64 -arch x86_64 -O2 -Wall -Wextra \
    "-DFOO2ZJS_PATH=\"$TMP/foo2zjs\"" \
    -o "$TMP/rastertozjs" "$HERE/filter/rastertozjs.c" -lcups
sed "s@/Library/Printers/foo2zjs-p1566/filter/rastertozjs@$TMP/rastertozjs@" \
    "$HERE/ppd/HP-LaserJet-P1566-foo2zjs.ppd" > "$TMP/test.ppd"

cupsfilter -e -i text/plain -m printer/foo -p "$TMP/test.ppd" \
    -o PageSize=A4 "$HERE/tests/test-page.txt" \
    > "$TMP/test.zjs" 2> "$TMP/filter.log" || {
        cat "$TMP/filter.log" >&2
        exit 1
    }

SIZE=$(wc -c < "$TMP/test.zjs" | tr -d ' ')
[ "$SIZE" -gt 0 ] || { cat "$TMP/filter.log" >&2; exit 1; }
grep -aq JZJZ "$TMP/test.zjs" || { cat "$TMP/filter.log" >&2; exit 1; }
[ "$SIZE" -gt 1024 ]
echo "PASS: macOS text → PDF → CUPS Raster → ZjStream ($SIZE bytes)"

# A real print job can span pages. The encoder must keep the page boundaries.
awk 'BEGIN { for (i = 1; i <= 140; i++) printf "P1566 multi-page line %03d\n", i }' \
    > "$TMP/multi.txt"
cupsfilter -i text/plain -m application/vnd.cups-raster -p "$TMP/test.ppd" \
    -o PageSize=A4 "$TMP/multi.txt" > "$TMP/multi.raster" 2> "$TMP/raster.log"
"$TMP/rastertozjs" 2 smoke multi 1 PageSize=A4 "$TMP/multi.raster" \
    > "$TMP/multi.zjs" 2> "$TMP/encoder.log"
grep -q 'PAGE: 2 1' "$TMP/encoder.log"
grep -aq JZJZ "$TMP/multi.zjs"
echo "PASS: multi-page CUPS Raster → ZjStream"

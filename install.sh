#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEST=/Library/Printers/foo2zjs-p1566
PPDDIR=/Library/Printers/PPDs/Contents/Resources
PPDNAME=HP-LaserJet-P1566-foo2zjs.ppd
QUEUE=HP_LaserJet_Pro_P1566
MAKE_QUEUE=1

usage() {
    echo "Usage: sudo ./install.sh [--no-queue] [--queue NAME]"
    exit 0
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --no-queue) MAKE_QUEUE=0; shift ;;
        --queue)
            [ "$#" -ge 2 ] || { echo "Missing queue name" >&2; exit 2; }
            QUEUE=$2; shift 2 ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

case "$QUEUE" in
    ''|*[!A-Za-z0-9_-]*) echo "Queue name must contain only letters, digits, _ or -." >&2; exit 2 ;;
esac

[ "$(uname -s)" = Darwin ] || { echo "This installer requires macOS." >&2; exit 1; }
[ "$(id -u)" -eq 0 ] || { echo "Run with sudo: sudo ./install.sh" >&2; exit 1; }
[ -x "$HERE/build/foo2zjs" ] && [ -x "$HERE/build/rastertozjs" ] ||
    { echo "Run ./build.sh first." >&2; exit 1; }

install -d -o root -g wheel -m 755 "$DEST/bin" "$DEST/filter" "$PPDDIR"
install -o root -g wheel -m 755 "$HERE/build/foo2zjs" "$DEST/bin/foo2zjs"
install -o root -g wheel -m 755 "$HERE/build/rastertozjs" "$DEST/filter/rastertozjs"
install -o root -g wheel -m 644 "$HERE/ppd/$PPDNAME" "$PPDDIR/$PPDNAME"

# Verify the exact installed filter path and the macOS conversion chain.
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
if ! cupsfilter -e -i text/plain -m printer/foo -p "$PPDDIR/$PPDNAME" \
        -o PageSize=A4 "$HERE/tests/test-page.txt" > "$TMP/test.zjs" 2> "$TMP/filter.log"; then
    cat "$TMP/filter.log" >&2
    echo "Installed driver self-test failed." >&2
    exit 1
fi
if ! grep -aq JZJZ "$TMP/test.zjs" ||
        [ "$(wc -c < "$TMP/test.zjs" | tr -d ' ')" -lt 1024 ]; then
    cat "$TMP/filter.log" >&2
    echo "Installed driver produced no ZjStream page." >&2
    exit 1
fi
echo "Driver installed and raster conversion verified."

if [ "$MAKE_QUEUE" -eq 1 ]; then
    URI=$(/usr/libexec/cups/backend/usb 2>/dev/null |
          awk 'tolower($0) ~ /p1566/ {print $2; exit}' || true)
    if [ -n "$URI" ]; then
        lpadmin -p "$QUEUE" -v "$URI" -P "$PPDDIR/$PPDNAME" \
            -D "HP LaserJet Pro P1566" -L USB -o printer-is-shared=false -E
        cupsenable "$QUEUE"
        cupsaccept "$QUEUE"
        echo "USB print queue ready: $QUEUE"
    else
        echo "P1566 is not connected. Connect it via USB, then run:"
        echo "  sudo ./install.sh --queue $QUEUE"
    fi
fi

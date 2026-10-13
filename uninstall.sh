#!/bin/sh
set -eu

[ "$(id -u)" -eq 0 ] || { echo "Run with sudo: sudo ./uninstall.sh" >&2; exit 1; }

DEST=/Library/Printers/foo2zjs-p1566
PPD=/Library/Printers/PPDs/Contents/Resources/HP-LaserJet-P1566-foo2zjs.ppd

rm -f "$DEST/bin/foo2zjs" "$DEST/filter/rastertozjs" "$PPD"
rmdir "$DEST/bin" "$DEST/filter" "$DEST" 2>/dev/null || true

echo "Driver files removed. If you created a print queue, remove it in"
echo "System Settings > Printers & Scanners."

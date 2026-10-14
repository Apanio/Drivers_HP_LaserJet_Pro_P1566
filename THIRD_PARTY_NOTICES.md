# Third-party source and license notices

This repository includes source from other open-source projects. The original
authors retain their notices and license terms. The GNU General Public License
version 2 text is in [LICENSE](LICENSE).

## `foo2zjs` and JBIG source in `src/`

The six files `foo2zjs.c`, `zjs.h`, `jbig.c`, `jbig.h`, `jbig_ar.c`, and
`jbig_ar.h` are byte-for-byte copies of the corresponding files in
[cragkhit/hp-p1102-macos at commit
`03d8cdffac61df6944d301aaaf32ac5d4f4681c3`](https://github.com/cragkhit/hp-p1102-macos/tree/03d8cdffac61df6944d301aaaf32ac5d4f4681c3/src).
That project packages the GPL-licensed `foo2zjs` encoder and JBIG-KIT for
macOS. Its repository identifies the license as GPL-2.0. The source comments
name Robert Szalai and Rick Richardson for `foo2zjs`, and Markus Kuhn for
JBIG-KIT. These files have not been modified in this repository.

## P1566 PPD

`ppd/HP-LaserJet-P1566-foo2zjs.ppd` is adapted from
[`PPD/HP-LaserJet_Pro_P1566.ppd` in OpenPrinting/foo2zjs at commit
`80499ed5bf6caa2963ad337e37cfda78a80aab1e`](https://github.com/OpenPrinting/foo2zjs/blob/80499ed5bf6caa2963ad337e37cfda78a80aab1e/PPD/HP-LaserJet_Pro_P1566.ppd).
The original PPD states that it is published under the GNU GPL. This version
uses the macOS CUPS raster filter, sets A4 as the default, and adds the
1200 × 600 dpi monochrome raster settings and updated model names.

## Project-specific files

`filter/rastertozjs.c`, the shell scripts, documentation, and test data were
created for this P1566 project and are distributed under GPL-2.0-or-later.
No printer firmware or prebuilt third-party binaries are included.

# HP LaserJet Pro P1566 driver for macOS

[Slovensky](README.sk.md)

A community CUPS driver for the USB HP LaserJet Pro P1566. It converts macOS
CUPS Raster output into the printer's ZjStream format using `foo2zjs`. The
driver is built from source on your Mac; it does not require Ghostscript,
Homebrew, or Rosetta at runtime.

**Verified:** One physical page printed successfully over USB on macOS 27.0.1
with Apple Silicon, using an existing CUPS queue switched to this driver. The
build and conversion smoke test also pass on that system. Multi-page output
was checked through the raster conversion pipeline, but not on the physical
printer. Intel Macs and other macOS versions have not been tested.

This is a community project. It is not affiliated with or endorsed by HP or
Apple.

## Requirements

- An HP LaserJet Pro P1566 connected by USB and switched on
- macOS with the CUPS PDF-to-raster filters
- Xcode Command Line Tools (`xcode-select --install`)
- Administrator access for installation

The build creates universal arm64 and x86_64 binaries. Only arm64 has been
tested with a printer.

## Install with a new print queue

Connect and switch on the printer, then run:

```sh
git clone https://github.com/Apanio/Drivers_HP_LaserJet_Pro_P1566.git
cd Drivers_HP_LaserJet_Pro_P1566
./build.sh
sudo ./install.sh
```

The installer places two executables in `/Library/Printers/foo2zjs-p1566`,
installs the PPD in `/Library/Printers/PPDs/Contents/Resources`, and verifies
the local print conversion. If the printer is detected over USB, it creates
the unshared `HP_LaserJet_Pro_P1566` queue. If USB detection fails, the driver
files are installed but no queue is created. Connect the printer and rerun
`sudo ./install.sh`.

Print a test page after the queue appears:

```sh
lp -d HP_LaserJet_Pro_P1566 tests/test-page.txt
```

## Use an existing P1566 queue

Installing driver files does not automatically change the driver of an
existing queue. To keep its name and USB address, replace `YOUR_QUEUE` with
the name shown by `lpstat -p`:

```sh
QUEUE=YOUR_QUEUE
./build.sh
sudo cp "/etc/cups/ppd/${QUEUE}.ppd" "/etc/cups/ppd/${QUEUE}.ppd.pre-foo2zjs"
sudo ./install.sh --no-queue
sudo lpadmin -p "$QUEUE" -P /Library/Printers/PPDs/Contents/Resources/HP-LaserJet-P1566-foo2zjs.ppd
lp -d "$QUEUE" tests/test-page.txt
```

The backup lets you restore the previous driver with:

```sh
sudo lpadmin -p "$QUEUE" -P "/etc/cups/ppd/${QUEUE}.ppd.pre-foo2zjs"
```

## Verify and troubleshoot

Run the conversion test without sending anything to the printer:

```sh
./tests/smoke.sh
```

If the installer does not find the printer, check the USB connection and
queue configuration:

```sh
/usr/libexec/cups/backend/usb
lpstat -v
```

If a job is processed but the printer does not print, power-cycle the printer
and inspect `/var/log/cups/error_log`. The physical test so far covers one
page; please report other paper sizes and multi-page results as they are
tested.

## Uninstall

If you switched an existing queue, restore its previous PPD first using the
backup command above. If the installer created a new queue, remove it with
`sudo lpadmin -x HP_LaserJet_Pro_P1566`. Then run `sudo ./uninstall.sh` to
remove the driver files.

## How it works and licensing

macOS renders the document as PDF, `cgpdftoraster` produces CUPS Raster, the
filter in `filter/rastertozjs.c` converts it to monochrome PBM, and `foo2zjs`
encodes ZjStream for the P1566. The printer uses the `-P -z2 -L0` mode.

The source and adapted PPD are distributed under the GNU GPL; see [LICENSE](LICENSE)
and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for upstream authors,
exact source revisions, and changes. This repository does not contain
prebuilt binaries or printer firmware.

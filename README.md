# HP LaserJet Pro P1566 pre macOS Golden Gate 27

Ovládač tlače pre HP LaserJet Pro P1566 na Macoch s Apple Silicon. Je postavený
na otvorenom enkóderi [foo2zjs](https://github.com/OpenPrinting/foo2zjs) a
natívnom rastrovom filtri CUPS. macOS prevedie PDF na CUPS Raster; náš filter
vytvorí čiernobiely obraz a foo2zjs ho zakóduje do formátu ZjStream, ktorému
P1566 rozumie.

**Stav:** zostavenie a prevod skúšobnej stránky boli overené na macOS 27.0.1
(arm64). Tlač na fyzickej P1566 ešte nebola overená, pretože tlačiareň pri
príprave ovládača nebola pripojená.

## Inštalácia

V Termináli otvorte tento priečinok a spustite:

    ./build.sh
    sudo ./install.sh

Potrebujete Xcode Command Line Tools. Skript nainštaluje dva malé spustiteľné
súbory do /Library/Printers/foo2zjs-p1566 a PPD do
/Library/Printers/PPDs/Contents/Resources. Ghostscript, Homebrew ani Rosetta
nie sú potrebné. Inštalátor spustí skúšobný prevod a zastaví sa pri chybe.

Ak je P1566 zapnutá a pripojená cez USB, inštalátor vytvorí tlačovú
frontu HP_LaserJet_Pro_P1566 bez zdieľania v sieti. Ak ju pripojíte neskôr
a ešte nemáte frontu, znovu spustite:

    sudo ./install.sh

Ak už v systéme existuje fronta pre P1566, inštalácia súborov na disk jej
ovládač automaticky nezmení. Na tomto Macu sa existujúca fronta volá
`HP_LaserJet_Professional_P1566`. Jej PPD môžete prepnúť bez vytvorenia
ďalšej fronty:

    sudo lpadmin -p HP_LaserJet_Professional_P1566 -P /Library/Printers/PPDs/Contents/Resources/HP-LaserJet-P1566-foo2zjs.ppd

Pred prepnutím si odložte pôvodný súbor
`/etc/cups/ppd/HP_LaserJet_Professional_P1566.ppd`. Na tomto Macu je záloha
v `local-backup/`; tento priečinok sa nenahráva na GitHub.

Skúšobná tlač:

    lp -d HP_LaserJet_Pro_P1566 tests/test-page.txt

Ak používate existujúcu frontu na tomto Macu:

    lp -d HP_LaserJet_Professional_P1566 tests/test-page.txt

V prípade neúspechu skontrolujte, či Mac tlačiareň vidí:

    /usr/libexec/cups/backend/usb
    lpstat -v

Ak sa tlačiareň v zozname USB vôbec neobjaví, problém je v spojení alebo v
režime HP Smart Install. Ak sa úloha spracuje, ale tlačiareň bliká, vypnite
a znovu zapnite tlačiareň a pozrite /var/log/cups/error_log.

## Odstránenie

    sudo ./uninstall.sh

Prípadnú tlačovú frontu potom odstráňte v Systémových nastaveniach.

## Technické poznámky

- P1566 používa model foo2zjs -z2 -P -L0; údaje o formáte a PPD
  pochádzajú z [OpenPrinting](https://github.com/OpenPrinting/foo2zjs).
- Súbor PPD vychádza z PPD/HP-LaserJet_Pro_P1566.ppd projektu OpenPrinting.
- Zdroj enkódera a JBIG knižnice v src/ pochádza z projektu
  [cragkhit/hp-p1102-macos](https://github.com/cragkhit/hp-p1102-macos),
  odvodeného od foo2zjs. Zachovaná licencia je v [LICENSE](LICENSE).
- Filter filter/rastertozjs.c je špecifická úprava pre P1566 a používa
  vstavaný cgpdftoraster namiesto Ghostscriptu.

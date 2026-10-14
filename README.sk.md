# Ovládač HP LaserJet Pro P1566 pre macOS

[English](README.md)

Komunitný ovládač CUPS pre tlačiareň HP LaserJet Pro P1566 pripojenú cez USB.
Raster z macOS prevádza pomocou `foo2zjs` do formátu ZjStream. Ovládač sa
zostaví zo zdrojového kódu na vašom Macu; pri používaní nepotrebuje
Ghostscript, Homebrew ani Rosettu.

**Overené:** jedna fyzická strana sa úspešne vytlačila cez USB na macOS 27.0.1
s Apple Silicon cez existujúcu frontu prepnutú na tento ovládač. Zostavenie a
skúšobný prevod tiež prešli. Viacstranový prevod bol overený bez fyzickej
tlače; Intel Macy a iné verzie macOS zatiaľ neboli odskúšané.

Ide o komunitný projekt bez spojenia so spoločnosťami HP a Apple.

## Požiadavky

- Zapnutá tlačiareň HP LaserJet Pro P1566 pripojená cez USB
- macOS s filtrami CUPS na prevod PDF do rastra
- Xcode Command Line Tools (`xcode-select --install`)
- Práva správcu na inštaláciu

Zostavujú sa univerzálne binárne súbory arm64 a x86_64. Tlač bola overená len
na arm64.

## Inštalácia s novou tlačovou frontou

Pripojte a zapnite tlačiareň, potom v Termináli spustite:

```sh
git clone https://github.com/Apanio/Drivers_HP_LaserJet_Pro_P1566.git
cd Drivers_HP_LaserJet_Pro_P1566
./build.sh
sudo ./install.sh
```

Inštalátor uloží dva spustiteľné súbory do `/Library/Printers/foo2zjs-p1566`,
PPD do `/Library/Printers/PPDs/Contents/Resources` a overí prevod. Ak tlačiareň
nájde cez USB, vytvorí nezdieľanú frontu `HP_LaserJet_Pro_P1566`. Ak ju
nenájde, nainštaluje iba súbory. Po pripojení tlačiarne spustite
`sudo ./install.sh` znova.

Skúšobná tlač:

```sh
lp -d HP_LaserJet_Pro_P1566 tests/test-page.txt
```

## Už existujúca fronta P1566

Samotná inštalácia súborov nemení ovládač existujúcej fronty. Namiesto
`YOUR_QUEUE` zadajte názov z `lpstat -p`:

```sh
QUEUE=YOUR_QUEUE
./build.sh
sudo cp "/etc/cups/ppd/${QUEUE}.ppd" "/etc/cups/ppd/${QUEUE}.ppd.pre-foo2zjs"
sudo ./install.sh --no-queue
sudo lpadmin -p "$QUEUE" -P /Library/Printers/PPDs/Contents/Resources/HP-LaserJet-P1566-foo2zjs.ppd
lp -d "$QUEUE" tests/test-page.txt
```

Pôvodný ovládač obnovíte zo zálohy:

```sh
sudo lpadmin -p "$QUEUE" -P "/etc/cups/ppd/${QUEUE}.ppd.pre-foo2zjs"
```

## Overenie a riešenie problémov

Skúšobný prevod bez fyzickej tlače:

```sh
./tests/smoke.sh
```

Ak inštalátor nenájde tlačiareň, skontrolujte USB a tlačové fronty:

```sh
/usr/libexec/cups/backend/usb
lpstat -v
```

Ak sa úloha spracuje, ale netlačí, vypnite a zapnite tlačiareň a pozrite
`/var/log/cups/error_log`. Fyzicky je zatiaľ overená jedna strana.

## Odstránenie

Pri pôvodnej fronte najprv obnovte pôvodné PPD zo zálohy. Novú frontu môžete
odstrániť príkazom `sudo lpadmin -x HP_LaserJet_Pro_P1566`. Potom spustite
`sudo ./uninstall.sh` na odstránenie súborov ovládača.

## Technické údaje a licencia

macOS vytvorí PDF, `cgpdftoraster` ho prevedie na CUPS Raster, filter
`filter/rastertozjs.c` vytvorí čiernobiely PBM a `foo2zjs` zakóduje ZjStream.
P1566 používa režim `-P -z2 -L0`.

Zdrojový kód a upravené PPD sa šíria pod GNU GPL. Pôvod autorov, presné
revízie a úpravy sú v [LICENSE](LICENSE) a
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Repozitár neobsahuje
predzostavené binárne súbory ani firmvér tlačiarne.

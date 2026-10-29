# Time Roll (Zeitprisma) – Projektzeit-Würfel mit M5Stack StickS3

## Zusammenarbeit
- Antworten auf Deutsch, kurz und präzise. Einschränkungen und Voraussetzungen zuerst nennen.
- Realistisch bewerten (Praxistauglichkeit), nicht nur Datenblattwerte. Unsicheres als unsicher kennzeichnen.
- Nichts als „getestet“ bezeichnen, was nur gegen Stubs oder am PC geprüft wurde.

## Ziel und Bauform
TimeFlip-artiger Zeiterfasser. **Standard: 8-Eck-„Münze“**, 53,8 mm Schlüsselweite, 22 mm dick. Der StickS3 liegt flach in einer Tasche, das Display zeigt nach vorne (1,5 mm vertieft).
- Steht die Münze auf dem Rand, ist die **oben** liegende Randseite (1–8) das aktive Projekt.
- Auf dem Rücken liegend (Display oben) = Pause. Display unten oder eine unklare Lage = ebenfalls Pause.
- Alternativen per Build-Flag: 12-Eck-Münze (`muenze12`), Zehnkant-Prisma (`prisma10`).

## Hardware (verifiziert, mit Quelle)
- StickS3 (K150): ESP32-S3-PICO-1-N8R8, BMI270 (I2C 0x68), PMIC M5PM1 (0x6E), ST7789P3 135×240, Akku 250 mAh.
  - Interner I2C: SDA G47, SCL G48. KEY1 = G11 (BtnA, vorne), KEY2 = G12.
  - M5PM1: PYG1_IRQ → G13, PYG4 ← BMI270 INT1, PYG2 = L3B-Schalter (Backlight/Mic/Lautsprecher), PYG3 = Lautsprecher-Verstärker.
  - Quellen: https://docs.m5stack.com/en/core/StickS3 und https://docs.m5stack.com/en/arduino/m5sticks3/m5pm1
- Stromangaben M5: L2-Zustand (ESP schläft) ca. 102 µA, ausgeschaltet ca. 14 µA.
- **BMI270 nach `M5.begin()`: `INT_MAP_DATA` (0x58) = 0xFF** (Daten-bereit-, FIFO- und Fehler-Interrupts auf INT1/INT2, am Gerät am 29.10.2026 zurückgelesen). Vor dem Armieren von any-motion muss das auf 0 (macht `bmiArmAnyMotion()`), sonst pulst INT1 mit 50 Hz und der Stick wacht nach dem Einschlafen sofort wieder auf. Die any-motion-Feature-Wörter sind nur bei ausgeschaltetem Advanced-Power-Save lesbar.
- M5PM1-GPIO-IRQ-Status wird flankengetriggert gesetzt und hält G13 Low, bis er gelöscht ist. Vor dem Schlafen G13 mindestens 300 ms ruhig High sehen (`waitIrqQuiet()`).
- **M5Unified-IMU-Init kostet Bootzeit:** `M5.begin()` initialisiert das BMI270 jedes Mal neu (Reset, 8-KB-Konfiguration, `INT_MAP_DATA`, laut `BMI270_Class.inl`). Beim Wecken aus dem Deep Sleep ist das überflüssig: `cfg.internal_imu = false` plus eigenes Lesen in `hal` (Rohwerte 0x0C bis 0x11, Achsen `{Y, −X, Z}`) spart am Gerät gemessen ca. 230 ms (`begin` 664 → 434 ms, Startzeile 0,80 → 0,57 s nach dem Chip-Start). Der Schnellstart gilt nur nach bestandenem `imuSelfCheck()` (RTC-Merker `imuFastOk`), sonst läuft alles wie bisher. Im Warmstart darf kein Code `M5.Imu` direkt aufrufen, nur `hal::readAccel()`.
- M5Unified remappt die IMU-Achsen für den StickS3 (`IMU_Class.inl`: „X=+Y, Y=-X, Z=+Z“). Wie Displayrotation und Achsen zusammenhängen, ist **nicht** dokumentiert → `DISP_ROT_OFFSET/DIR` in `config.h`.
- Maße aus dem offiziellen Strukturmodell (`make ref` → `ref/StickS3.stl`):
  - 48,0 × 24,0 × 14,08 mm; Taste A ragt 0,92 mm vor
  - USB-C-Mitte bei sx = 11,7 / sz = −4,25 mm (±0,5, aus dem Modell abgelesen)
  - Magnet hinten mittig bei sy = 24,4
  - Im Modell fehlt das Displayglas.

## Struktur
```
firmware/                    PlatformIO (Arduino, M5Unified, M5PM1), Envs muenze8 | muenze12 | prisma10
  include/config.h           Projektnamen, WLAN-Platzhalter, Erkennung (Zeiten, Ringparameter), Display-Rotation, Strom
  include/config_local.h     echte WLAN-Zugangsdaten (gitignored, Vorlage config_local.h.example)
  lib/prisma_core/           hardwareunabhängig: Seitenerkennung, Ruhe, Entprellen, Teach-in,
                             Tagessummen, CSV  (am PC testbar)
  src/hal.*                  M5PM1, BMI270 any-motion, Deep/Light Sleep, Weckkette
  src/store.*                LittleFS-Log /log.csv, Seitenmodell in NVS (versioniert je Bauform)
  src/net.*                  NTP, Web-Export http://zeitprisma.local
  src/ui.*                   Anzeige, Tonfolgen
  src/main.cpp               Ablauf, RTC-Zustand (Magic je Bauform)
  test/host/                 Host-Tests (g++), alle drei Bauformen
gehaeuse/muenze_gehaeuse.py  N-Eck-Münze (--seiten, --dicke, --usb auto|ja|nein, --check)
gehaeuse/prisma_gehaeuse.py  Zehnkant-Prisma (Körper + Deckel)
gehaeuse/*.stl, *.png        zeitprisma_muenze8_22mm_usb.stl, vorschau_muenze8.png
                             (alter Sechskant, 5 Projekte: gelöscht, nur noch im Git-Verlauf, Commit a231673)
Makefile                     test | build | flash | log | ref | gehaeuse | alle-gehaeuse | clean (löscht auch firmware/.pio!)
tools/auswertung.py          CSV → Stunden je Projekt/Tag/Woche
tools/seriallog.py           serielle Ausgabe nicht-interaktiv mitschreiben (verbindet nach Deep Sleep neu)
```

## Befehle
```
python3 -m venv .venv && . .venv/bin/activate
pip install platformio manifold3d numpy matplotlib pyserial   # geprüft mit manifold3d 3.5.4
make test                  # Host-Tests (Pflicht nach Änderungen an prisma_core)
make build                 # pio run -e muenze8 -e muenze12 -e prisma10 (ohne -e baut pio nur muenze8)
make flash ENV=muenze8     # flashen
make log                   # 30 s serielle Ausgabe (tools/seriallog.py)
make ref && make gehaeuse  # Gehäuse erzeugen und gegen das M5-Modell prüfen
```
`pio device monitor` ist interaktiv; zum Mitlesen besser `tools/seriallog.py -s <Sekunden>` verwenden.

## Konventionen
- **I2C nur über `M5.In_I2C`** (m5::I2C_Class), auch für M5PM1 (`pm1.begin(&M5.In_I2C)`) und BMI270. **Kein Arduino-`Wire`** auf G47/G48: lgfx-I2C legt die Pins bei jeder Transaktion neu auf seinen Port.
- Neue Logik möglichst in `lib/prisma_core` und mit Test in `test/host/test_core.cpp`. Die Tests müssen für alle drei Bauformen laufen.
- Bauform-Konstanten: `kCoin`, `kRing`, `kFaces`, `kStepDeg`, `kRingFirst`. Seitenerkennung im Betrieb: `classifyRing()` mit `RingParams` (keine Totzone; Hysterese `RING_HYST_DEG` = 0,2 · kStepDeg, Ringebene 35°/45°, Seite 0 30°/40°; Werte in `config.h`). `classify()` mit Fangbereich bleibt nur für Tests.
- Texte am Display in ASCII (keine Umlaute, die Fonts haben sie nicht). Code-Kommentare ebenfalls ASCII.
- Gehäuse: Nach jeder Parameteränderung `--check ref/StickS3.stl` ausführen. Mindestwand 1,6 mm; Taste A braucht ≥ 0,3 mm Luft zur Vorderseite.
- Gedruckt wird ohne Stützen. Die Münze liegt beim Druck mit der Rückseite auf dem Bett; einzige Brücke ist das USB-Loch (13,5 mm).
- **WLAN-Zugangsdaten nie in getrackte Dateien** (das Repo ist öffentlich auf GitHub). Sie stehen in `firmware/include/config_local.h` (gitignored); `config.h` bindet sie per `__has_include` ein. Beim Integrieren einer neuen ZIP prüfen, dass `config.h` diesen Block noch hat, und nur gezielte Pfade stagen.

## Stand
**Auf Hardware bestätigt: App startet, Wecken per Bewegung, Seitenwechsel beim Rollen von Hand (26 Wechsel im letzten Mitschnitt, kein „unbekannt“, erste Seite 1,0 bis 1,3 s nach dem Chip-Start). Nicht getestet: Teach-in mit Gehäuse, Töne, Tasten, Strom. Gehäuse nicht gedruckt.** Am StickS3 belegt ist nur der Block „Am Gerät“ unten, alles andere ist nur am PC geprüft.

Ende der claude.ai-Sitzung (28.10.2026): Firmware dort nie mit der ESP32-Toolchain gebaut (kein Zugriff), geprüft nur gegen Stub-Header mit den echten M5Unified-/M5PM1-Signaturen und per Host-Tests.

In Claude Code nachgeprüft (29.10.2026):
- `make test`: Münze 8 = 236, Münze 12 = 326, Prisma 10 = 255 Prüfungen, alle grün, keine Warnungen (`-Wall -Wextra`). Stand der ZIP war 130/188/140; die Mehrzahl sind die neuen Tests der Ringerkennung. Mutationstest: ohne Hysterese fallen 24, mit wieder eingebauter Totzone 28 Prüfungen durch.
- `make build`: `muenze8`, `muenze12`, `prisma10` kompilieren und linken (PlatformIO 6.2.0, espressif32@6.12.0), 0 Warnungen. `muenze8`: RAM 15,8 %, Flash 34,0 %. Das heißt nur „baut“.
- Gehäuse: `--check` gegen das M5-Modell besteht für Münze 8 (10 Punkte), Münze 12 (8) und Prisma 10 (14). Die gelieferte STL `zeitprisma_muenze8_22mm_usb.stl` entspricht dem Skript (gleiches Volumen, Eckpunkte um höchstens 0,001 mm verschieden), misst 53,80 × 53,80 × 22,00 mm und ist wasserdicht.
- Kippgrenzen (gerechnet, Stick-Schwerpunkt = Mitte angenommen): Münze 8 ab 21,0° (flach um ab 20,1°), Münze 12 ab 13,4°, Prisma 10 je nach Seite ab 12,2–18,5°.
- Die Gehäuse-Skripte brauchten eine Korrektur: `CrossSection.even_odd` gibt es in manifold3d 3.5.4 nicht, jetzt `CrossSection(..., FillRule.EvenOdd)`.

Am Gerät (29.10.2026):
- Chip laut esptool: ESP32-S3-PICO-1 (LGA56) v0.2, 8 MB Flash, 8 MB PSRAM, USB-Serial/JTAG (`303a:1001`, `/dev/ttyACM0`).
- Vor dem Flashen startete der Stick im Sekundentakt neu (Kernel-Log: ständig neue USB-Gerätenummern). Sehr wahrscheinlich die Wecken-Schleife (siehe unten), nicht nachgestellt. Der Download-Modus (seitliche Reset-Taste halten, bis die grüne LED blinkt, laut M5-Doku) machte ihn stabil.
- `make flash ENV=muenze8` schrieb Bootloader, Partitionstabelle, boot_app0 und App (1.135.744 Bytes), alle Hashes verifiziert.
- Nach dem Flashen blieb der Stick im Bootloader: Der RTS-Hard-Reset von esptool führt hier zu `boot:0x21 (DOWNLOAD(USB/UART0))`. Ein **Watchdog-Reset startet die App** (`esptool.py ... --before default_reset --after watchdog_reset chip_id`).
- Startzeile gesehen: `Start: kalt=1 Weckgrund=0 Seite=-1 Modell=0 Akku=4170 mV Bauform=Muenze/9`. Keine Fehlermeldung zu M5PM1/BMI270, Akkuspannung aus dem M5PM1 gelesen, WLAN offenbar verbunden (keine Warnung „WLAN nicht erreichbar“). NTP-Ergebnis, Teach-in, Seitenerkennung, Wecken per Bewegung, Tasten, Töne und Strom sind **nicht getestet**.
- ModemManager hielt den Port offen (der Port wanderte von `ttyACM0` auf `ttyACM1`), PlatformIO meldet veraltete udev-Regeln (`99-platformio-udev.rules`). Das erklärt die wandernde Port-Nummer vermutlich, nicht die Neustart-Schleife. Befehl zum Erneuern: README, Abschnitt Flashen.
- Neue Ringerkennung (geflasht): Der erste Seitenwechsel kam 0,85 s nach Beginn der Abtastung (Log `Seite 5 -> Projekt 5`), das Teach-in-Modell aus dem NVS blieb erhalten (`Modell=1`). Im 200-s-Mitschnitt mit Rollen von Hand (ohne Gehäuse): 24 Seitenwechsel (Seiten 1, 3, 5, 6, 7, 8 und Pause), kein „unbekannt“, Wechsel im selben Wachabschnitt im Mittel 3,9 s auseinander (kürzester 2,5 s, das ist das Rolltempo), erster Wechsel nach dem Wecken 1,6 bis 2,4 s nach dem Chip-Start (ca. 0,8 s Boot). Wie es sich anfühlt, bewertet der Nutzer.
- **Wecken-Schleife, Ursache gefunden und behoben (29.10.2026):** Der Stick wachte nach dem Einschlafen ca. 0,6 s später sofort wieder auf (`Weckgrund=2`), dreimal, dann Rückfall „Timer 60 s“, auch bei unberührtem Stick. Ursache: BMI270 `INT_MAP_DATA` stand nach `M5.begin()` auf 0xFF, INT1 pulste mit jedem Messwert (Trace: Impulse nur auf dem 20-ms-Raster), der M5PM1 hielt G13 per Flanken-Status auf Low. Fix in `bmiArmAnyMotion()` (`INT_MAP_DATA`, `INT2_MAP_FEAT`, `INT2_IO_CTRL` = 0) plus `waitIrqQuiet()` vor dem Schlafen. Gemessen mit `WAKE_DIAG 1`: 0 INT1-Impulse in 1,5 s, ruhig nach 300 ms, der Stick blieb im Deep Sleep. Details und Log-Format: README, „Diagnose der Weckkette“. **Bestätigt mit der Standard-Version (`WAKE_DIAG 0`, 200-s-Mitschnitt):** Bewegung weckt (4 Mal), unberührt schlief der Stick 84 s ohne Aufwachen, keine Schleife mehr.
- **Boot und Entprellung verkürzt (29.10.2026, geflasht, 200-s-Mitschnitt von Hand gerollt):** `STILL_WINDOW` 10 → 6 (0,12 s), `SETTLE_MS` 600 → 250 (Erkennung ca. 0,4 s statt 0,8 s nach dem Stillstand), Warmstart ohne M5-IMU-Init. Gemessen: Startzeile 572 bis 576 ms nach dem Chip-Start (vorher 802 bis 814 ms), `Boot ms: setup@64 begin=434 ui=0 hal=72 store=2..6 imu=eigen` (mit M5-Init `begin=664 hal=89`, Kaltstart `setup@247`). `IMU-Abgleich` beim Kaltstart abgelehnt (Stick in Bewegung, |g| 1,62), beim nächsten Start bestanden (|g| 1,00); beide Lesungen stimmten bei beiden Lagen auf 0,000 g überein, die Achsenabbildung `{Y, −X, Z}` passt also zu M5Unified. 26 Wechsel (alle 8 Randseiten und Pause), kein „unbekannt“, kürzester Abstand zweier Wechsel im selben Wachabschnitt 1,43 s (Rolltempo des Nutzers), erste Seite nach dem Wecken 1,0 bis 1,3 s nach dem Chip-Start (vorher 1,6 bis 2,4 s). 8 Mal per Bewegung geweckt, unberührt 35 s und 55 s Schlaf am Stück. Nicht geprüft: sehr schnelles Rollen (falsche Zwischenseiten mit 0,25 s Entprellung?), Strom, Verhalten im Gehäuse.
- Befehle und Erklärung zu allem Verhalten der Firmware: README (Flashen, Funktionsweise, Fehlersuche). Für Erklärungen zuerst die README lesen, nicht den Code.
- Die BMI270-any-motion-Register werden direkt geschrieben (`hal.cpp`, nach Bosch SensorAPI `bmi270.c`). Das M5-Beispiel nutzt dafür die SparkFun-Lib.

## Nächste Schritte (Reihenfolge)
1. Erledigt (29.10.2026): `make build`, alle drei Envs bauen. Bei neuen M5Unified-/M5PM1-Ständen erneut prüfen (`lib_deps` sind nicht gepinnt).
2. Erledigt (29.10.2026): geflasht, Startzeile gesehen, M5PM1/BMI270 ohne Fehler. Offen: udev-Regeln erneuern (README, Flashen) und prüfen, ob `make flash` die App danach von selbst startet.
3. Erledigt (29.10.2026): Weckkette geprüft, Wecken-Schleife behoben, Wecken per Bewegung am Gerät bestätigt (siehe Stand, „Am Gerät“). Offen: Empfindlichkeit im Alltag abstimmen (`ANYMOT_THRESHOLD`, `ANYMOT_DURATION`), Verhalten in der Tasche und im Gehäuse. Rückfall bleibt `POWER_DEEPSLEEP 0`.
4. Teach-in durchspielen, Erkennung aller 8 Seiten und der Pause prüfen. Ringerkennung am Gerät abstimmen (Rückmeldung 29.10.2026: Wechsel deutlich schneller, in der Drehachse immer ein Projekt, mit Hysterese): `RING_HYST_DEG`, `RING_UPRIGHT_DEG`, `SETTLE_MS` (jetzt 250), `STILL_WINDOW` (jetzt 6) in `config.h`; bei falschen Zwischenseiten beim schnellen Rollen `SETTLE_MS` erhöhen.
5. `DISP_ROT_OFFSET/DIR` so kalibrieren, dass der Text aufgestellt aufrecht steht.
6. Ruhestrom/Laufzeit: `vbat_mv` im Log über einige Tage beobachten.
7. Probedruck: Passung der Tasche, Magnet-Halt an der Scheibe (M8 DIN 125), USB-Stecker im Loch, Kippverhalten.

## Annahmen / offene Punkte
- Stick-Schwerpunkt (Lage des Akkus) unbekannt → die Stabilitätsrechnung nimmt die Mitte an.
- USB-Loch 13,5 × 7 mm: Stecker bis ca. 12,4 × 6,7 mm passen, dicke Umspritzungen eventuell nicht.
- Bei 8 Seiten liegen die schrägen Seiten (1, 3, 5, 7) genau zwischen zwei 90°-Displayrotationen. Der Text steht dann 45° schief.
- Laufzeit pro Akkuladung ist nur rechnerisch abgeschätzt (Wochen), nicht gemessen.
- `lib_deps` sind nicht gepinnt (GitHub-HEAD). Am 29.10.2026 aufgelöst: M5Unified 0.2.28 (565a499), M5GFX 0.2.35, M5PM1 1.0.7 (315d289). Der frühere Build vom 16.10. (Env `sticks3`) lief noch mit M5Unified 0.2.26 und M5GFX 0.2.33. Bricht ein Upstream-Update die API, auf diese Stände pinnen.
- Der Stick sitzt nur magnetisch in der offenen Tasche (kein mechanischer Halt). Ob der Magnetrücken an der M8-Scheibe beim Umdrehen und Rollen hält, ist ungeprüft.
- Die Seitentaster (Einschalten, Download-Modus) sind in der Münze verdeckt. Geflasht werden kann nur, solange der Stick wach ist: im Deep Sleep verschwindet der USB-Port (siehe `tools/seriallog.py`).

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
- M5Unified remappt die IMU-Achsen für den StickS3 (`IMU_Class.inl`: „X=+Y, Y=-X, Z=+Z“). Wie Displayrotation und Achsen zusammenhängen, ist **nicht** dokumentiert → `DISP_ROT_OFFSET/DIR` in `config.h`.
- Maße aus dem offiziellen Strukturmodell (`make ref` → `ref/StickS3.stl`):
  - 48,0 × 24,0 × 14,08 mm; Taste A ragt 0,92 mm vor
  - USB-C-Mitte bei sx = 11,7 / sz = −4,25 mm (±0,5, aus dem Modell abgelesen)
  - Magnet hinten mittig bei sy = 24,4
  - Im Modell fehlt das Displayglas.

## Struktur
```
firmware/                    PlatformIO (Arduino, M5Unified, M5PM1), Envs muenze8 | muenze12 | prisma10
  include/config.h           Projektnamen, WLAN-Platzhalter, Fangbereich, Display-Rotation, Strom
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
- Bauform-Konstanten: `kCoin`, `kRing`, `kFaces`, `kStepDeg`, `kRingFirst`. Fangbereich: `MATCH_DEG = 0,4 · kStepDeg`.
- Texte am Display in ASCII (keine Umlaute, die Fonts haben sie nicht). Code-Kommentare ebenfalls ASCII.
- Gehäuse: Nach jeder Parameteränderung `--check ref/StickS3.stl` ausführen. Mindestwand 1,6 mm; Taste A braucht ≥ 0,3 mm Luft zur Vorderseite.
- Gedruckt wird ohne Stützen. Die Münze liegt beim Druck mit der Rückseite auf dem Bett; einzige Brücke ist das USB-Loch (13,5 mm).
- **WLAN-Zugangsdaten nie in getrackte Dateien** (das Repo ist öffentlich auf GitHub). Sie stehen in `firmware/include/config_local.h` (gitignored); `config.h` bindet sie per `__has_include` ein. Beim Integrieren einer neuen ZIP prüfen, dass `config.h` diesen Block noch hat, und nur gezielte Pfade stagen.

## Stand
**Nicht auf Hardware getestet, Gehäuse nicht gedruckt.** Alles Folgende ist nur am PC geprüft.

Ende der claude.ai-Sitzung (28.10.2026): Firmware dort nie mit der ESP32-Toolchain gebaut (kein Zugriff), geprüft nur gegen Stub-Header mit den echten M5Unified-/M5PM1-Signaturen und per Host-Tests.

In Claude Code nachgeprüft (29.10.2026):
- `make test`: Münze 8 = 130, Münze 12 = 188, Prisma 10 = 140 Prüfungen, alle grün, keine Warnungen (`-Wall -Wextra`).
- `make build`: `muenze8`, `muenze12`, `prisma10` kompilieren und linken (PlatformIO 6.2.0, espressif32@6.12.0), 0 Warnungen. `muenze8`: RAM 15,8 %, Flash 34,0 %. Das heißt nur „baut“.
- Gehäuse: `--check` gegen das M5-Modell besteht für Münze 8 (10 Punkte), Münze 12 (8) und Prisma 10 (14). Die gelieferte STL `zeitprisma_muenze8_22mm_usb.stl` entspricht dem Skript (gleiches Volumen, Eckpunkte um höchstens 0,001 mm verschieden), misst 53,80 × 53,80 × 22,00 mm und ist wasserdicht.
- Kippgrenzen (gerechnet, Stick-Schwerpunkt = Mitte angenommen): Münze 8 ab 21,0° (flach um ab 20,1°), Münze 12 ab 13,4°, Prisma 10 je nach Seite ab 12,2–18,5°.
- Die Gehäuse-Skripte brauchten eine Korrektur: `CrossSection.even_odd` gibt es in manifold3d 3.5.4 nicht, jetzt `CrossSection(..., FillRule.EvenOdd)`.
- Die BMI270-any-motion-Register werden direkt geschrieben (`hal.cpp`, nach Bosch SensorAPI `bmi270.c`). Das M5-Beispiel nutzt dafür die SparkFun-Lib.

## Nächste Schritte (Reihenfolge)
1. Erledigt (29.10.2026): `make build`, alle drei Envs bauen. Bei neuen M5Unified-/M5PM1-Ständen erneut prüfen (`lib_deps` sind nicht gepinnt).
2. Flashen, `make log`: Erscheint die Startzeile „Start: kalt=… Weckgrund=…“? Wurden M5PM1 und BMI270 gefunden?
3. **Weckkette prüfen:** Deep Sleep → Münze bewegen → aufwachen (Weckgrund im Log). Wenn das nicht geht: Fehler suchen; Rückfall ist `POWER_DEEPSLEEP 0`.
4. Teach-in durchspielen, Erkennung aller 8 Seiten und der Pause prüfen (Fangbereich ±18°).
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

# Time Roll (Zeitprisma)

Projektzeit-Tracker im Stil von TimeFlip mit M5Stack StickS3, Fortsetzung des Projekts „M5StickC S3 Projektzeitschreiber“.
Entwicklung (Hardware-Fakten mit Quellen, Befehle, Konventionen, nächste Schritte): siehe `CLAUDE.md`.

> **Stand, Einschränkungen zuerst:** Nicht auf Hardware getestet, Gehäuse nicht gedruckt.
> Nur am PC geprüft (29.10.2026): Kernlogik in Host-Tests (Münze 8 = 130, Münze 12 = 188, Prisma 10 = 140 Prüfungen), Gehäuse-Passung gegen das M5-Strukturmodell (rechnerisch) und dass alle drei Firmware-Varianten mit der ESP32-S3-Toolchain kompilieren (`make build`). „Kompiliert“ heißt nicht „läuft“.

**Standard: 8-Eck-Münze**, 53,8 mm Schlüsselweite (ca. 57,6 mm über die gefasten Ecken) × 22 mm.
- Der Stick liegt flach in der Mitte, das Display zeigt nach vorne.
- Steht die Münze auf dem Rand, ist die **oben** liegende Randseite (1–8) das aktive Projekt.
- Auf dem Rücken liegend (Display oben) = Pause. Display unten oder eine unklare Lage = ebenfalls Pause.

| Variante | Gehäuse | Firmware-Env |
|---|---|---|
| **8-Eck-Münze, 22 mm, mit USB-C-Loch** | `zeitprisma_muenze8_22mm_usb.stl` | `muenze8` (Standard) |
| 12-Eck-Münze, 18,6 mm | `muenze_gehaeuse.py --seiten 12 --dicke 18.58 --usb nein` | `muenze12` |
| Zehnkant-Prisma (Körper + Deckel) | `prisma_gehaeuse.py` (erzeugt `zeitprisma10_koerper.stl`, `zeitprisma10_deckel.stl`) | `prisma10` |

```
gehaeuse/   muenze_gehaeuse.py, prisma_gehaeuse.py (parametrisch), fertige STL, Vorschau
firmware/   PlatformIO-Projekt (Arduino, M5Unified, M5PM1), Envs muenze8 | muenze12 | prisma10
tools/      auswertung.py (CSV -> Stunden je Projekt und Tag/Woche), seriallog.py (serielle Ausgabe mitschreiben)
Makefile    make test | build | flash | log | ref | gehaeuse
```

## Gehäuse (8-Eck-Münze)
- **Druck:** ein Teil, Rückseite aufs Bett, ohne Stützen. Einzige Brücke ist das USB-Loch (13,5 mm). Ca. 15 g PLA (gerechnet, 35 % Füllgrad angenommen; je nach Slicer-Einstellung vermutlich mehr).
- **Einbau:**
  1. M8-Unterlegscheibe DIN 125 (16 × 1,6 mm, Stahl) in die Mulde im Taschenboden legen, eventuell mit einem Tropfen Kleber. Sie hält den Stick über seinen Magnetrücken.
  2. Stick mit dem USB-Ende zur Seite „6“ einlegen.
  3. Herausnehmen: durch das Ø10-Loch in der Rückseite drücken.
- **Laden:** USB-C-Loch in Seite 6, 13,5 × 7 mm. Es passen Stecker bis ca. 12,4 × 6,7 mm.
- **Seitentaster** (Einschalten, Download-Modus) sind bei eingebautem Stick **nicht erreichbar**; die Münze hat dafür keine Löcher. Das Zehnkant-Prisma hat Ø5,5-Zugangslöcher.
- **Stabilität (gerechnet, nicht gemessen):** Aufgestellt kippt die Münze ab ca. 21° zur Nachbarseite und ab ca. 20° flach um. Zum Vergleich: die 12-Eck-Münze mit 18,6 mm kippte ab ca. 13°.
- **Ziffern:** 1–8 auf den Randflächen, 6. mit Punkt. Lesbar von vorne-oben, wenn das Display zu einem zeigt.
- **Anpassen:** `python3 muenze_gehaeuse.py --seiten N --dicke D --usb auto|ja|nein --check ../ref/StickS3.stl` (Abhängigkeiten: `pip install manifold3d numpy matplotlib`, geprüft mit manifold3d 3.5.4; das Referenzmodell lädt `make ref`).
- **Entfernt:** Das frühere Sechskant-Prisma (5 Projekte, `zeitprisma_gehaeuse.stl`, `vorschau.png`) ist gelöscht, weil kein Skript und keine Firmware-Variante es noch erzeugt oder nutzt. Es liegt im Git-Verlauf (Commit a231673).

## Firmware
- **Einrichten:**
  1. WLAN-Zugangsdaten in `firmware/include/config_local.h` eintragen (Vorlage: `config_local.h.example`; die Datei steht in `.gitignore` und kommt nie nach GitHub), Projektnamen in `firmware/include/config.h`.
  2. Flashen mit `make flash` bzw. `pio run -e muenze8 -t upload`. Download-Modus: Seitentaster lang drücken.
  3. Beim ersten Start: Uhrzeit per NTP holen, dann Teach-in. Dazu das Display nach oben legen und danach die Seiten 1…8 der Reihe nach nach oben stellen, jeweils ca. 1 s ruhig.
- **Seitenwechsel:** Nach 3 s Ruhe wird die neue Seite übernommen, mit einem Fangbereich von ±18°.
- **Töne:** 5 ist ein langer Ton, der Rest kurze (8 = lang + 3 kurz). Pause ist ein tiefer Ton.
- **Anzeige:** Ist die Münze aufgestellt, dreht sich der Text automatisch aufrecht. Die Richtung muss einmal kalibriert werden (`DISP_ROT_OFFSET/DIR`). Bei 8 Seiten steht der Text auf den schrägen Seiten (1, 3, 5, 7) um 45° schief.
- **Taste A:**
  - kurz: Tagesübersicht
  - 2 s halten: WLAN-Export unter `http://zeitprisma.local` mit `log.csv`
  - 6 s halten: Teach-in
- **Log:** `zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok`, eine Zeile je Wechsel.
- **Auswertung:** `python3 tools/auswertung.py zeitprisma.csv [--woche] [--csv out.csv]`

## Strom
`POWER_DEEPSLEEP 1`: Bewegung weckt den ESP32 über BMI270 any-motion → M5PM1 PYG4 → IRQ PYG1 → G13. Danach wird abgetastet, geloggt und wieder geschlafen. M5 gibt für diesen Board-Zustand (L2) ca. 102 µA an. Rechnerisch reicht das für einige Wochen pro Ladung, real bitte messen: `vbat_mv` wird mitgeloggt. Dauerwackeln (Tasche) → 5 min Timer-Schlaf. Rückfall `POWER_DEEPSLEEP 0`: Light Sleep mit Abfrage, einfacher, aber spürbar mehr Verbrauch.

## Offene Punkte (ungetestet)
- **Weckkette:** Die BMI270-any-motion-Register sind aus der Bosch-SensorAPI übernommen, die M5PM1-Konfiguration aus der M5-Doku. Beides muss auf der Hardware verifiziert werden.
- **Magnet-Halt:** Annahme „Magnetic back“ hält an Stahl. Der Stick sitzt nur magnetisch in der offenen Tasche, falls zu schwach: dünnes doppelseitiges Klebeband.
- **Display-Rotation:** `DISP_ROT_OFFSET/DIR` in `config.h` einmal am Gerät kalibrieren.
- **Geometrie:** Gehäusemaße aus dem offiziellen Strukturmodell (`M5_Hardware/Products/K150_StickS3`), Schwerpunkt des Sticks unbekannt (Annahme: Mitte), USB-Loch nur für schlanke Stecker.
- **Flashen im Deep Sleep:** Der USB-Port erscheint nur, solange der Stick wach ist (siehe `tools/seriallog.py`). Zum Flashen die Münze bewegen, bei eingebautem Stick gibt es keinen Seitentaster-Zugang.

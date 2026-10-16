# Time Roll (Zeitprisma)

Projektzeit-Tracker im Stil von TimeFlip mit M5Stack StickS3, Fortsetzung des Projekts „M5StickC S3 Projektzeitschreiber“.

Sechskant-Prisma, oben liegende Seite = aktives Projekt (wie TimeFlip).
Seite 0 (Display oben) oder aufrecht = Pause. Seiten 1–5 tragen Würfelaugen.

```
gehaeuse/   prisma_gehaeuse.py (parametrisch), zeitprisma_gehaeuse.stl, vorschau.png
firmware/   PlatformIO-Projekt (Arduino, M5Unified, M5PM1)
tools/      auswertung.py (CSV -> Stunden je Projekt und Tag/Woche)
```

## Gehäuse

- Maße: 51,6 mm Schlüsselweite × 53,4 mm Länge. Stick-Front liegt 1,5 mm unter Seite 0, Taste A hat 0,6 mm Luft zum Tisch.
- Druck: STL liegt mit Seite 3 auf dem Bett, keine Stützen nötig. PLA/PETG, 0,4er Düse, 3 Wände, 15–20 % Infill, ca. 40–60 g.
- Halt: M8-Unterlegscheibe DIN 125 (16 × 1,6 mm, Stahl) in die runde Mulde im Taschenboden legen, eventuell mit einem Tropfen Kleber. Der Magnetrücken des Sticks hält ihn darauf. Stick von vorne einlegen, Display nach außen, USB-C zur Seite mit der großen Aussparung.
- Herausnehmen: an den Stirnseiten-Aussparungen greifen.
- Seitentaster: über die beiden Ø5-Löcher mit einem Stift erreichbar (Einschalten, Download-Modus).
- Anpassen: Parameter oben in `prisma_gehaeuse.py` ändern (z. B. `FIT` für die Passung), dann `pip install manifold3d numpy` und `python3 prisma_gehaeuse.py`. Mit `--check StickS3.stl` (M5-Strukturmodell) läuft die Passungsprüfung.

## Firmware

1. In `firmware/include/config.h` WLAN und Projektnamen eintragen.
2. Flashen mit `pio run -t upload`. Download-Modus: Seitentaster lang drücken.
3. Erster Start: Uhrzeit per NTP holen, dann **Teach-in**: Display nach oben legen, danach der Reihe nach auf 1, 2 … 5 Augen oben rollen und jeweils ruhig liegen lassen. Nach jeder Seite kommen entsprechend viele Pieptöne.

**Bedienung**
- Rollen: Nach 3 s Ruhe wird die neue Seite übernommen. Rückmeldung: Pieptöne = Projektnummer, Pause = ein tiefer Ton.
- Taste A kurz: Tagesübersicht.
- Taste A 2 s halten: WLAN-Export unter `http://zeitprisma.local` mit `log.csv`.
- Taste A 6 s halten: Teach-in.

**Log:** `zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok`, eine Zeile je Wechsel. Auswertung:
`python3 tools/auswertung.py zeitprisma.csv [--woche] [--csv out.csv]`

**Strom (`POWER_DEEPSLEEP 1`):** Bewegung weckt den ESP32 über BMI270 any-motion → M5PM1 PYG4 → IRQ PYG1 → G13. Danach wird abgetastet, geloggt und wieder geschlafen. M5 gibt für diesen Board-Zustand (L2) ca. 102 µA an. Rechnerisch reicht das für einige Wochen pro Ladung, real bitte messen: `vbat_mv` wird mitgeloggt. Dauerwackeln (Tasche) → 5 min Timer-Schlaf. Rückfall `POWER_DEEPSLEEP 0`: Light Sleep mit Abfrage, einfacher, aber spürbar mehr Verbrauch.

## Stand / ungetestet

- **Nicht mit der ESP32-Toolchain kompiliert** (hier kein Zugriff). Geprüft wurden:
  - Syntax gegen Stub-Header mit den echten Signaturen aus M5Unified/M5PM1 (aktueller GitHub-Stand)
  - Kernlogik: 64 Host-Tests, `make -C firmware/test/host`
- **Weckkette:** Die BMI270-any-motion-Register sind aus der Bosch-SensorAPI übernommen, die M5PM1-Konfiguration aus der M5-Doku. Beides muss auf der Hardware verifiziert werden.
- **Magnet-Halt:** Annahme „Magnetic back“ hält an Stahl. Falls zu schwach: dünnes doppelseitiges Klebeband.
- **Geometrie:** Gehäusemaße aus dem offiziellen Strukturmodell (`M5_Hardware/Products/K150_StickS3`). Lage der Seitentaster vor dem Druck kurz am Gerät gegenprüfen.
- **Display-Rotation** eventuell auf `setRotation(3)` drehen.

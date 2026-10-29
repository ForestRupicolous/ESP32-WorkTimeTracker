# Time Roll (Zeitprisma)

Projektzeit-Tracker im Stil von TimeFlip mit M5Stack StickS3, Fortsetzung des Projekts „M5StickC S3 Projektzeitschreiber“.
Entwicklung (Hardware-Fakten mit Quellen, Befehle, Konventionen, nächste Schritte): siehe `CLAUDE.md`.

> **Stand, Einschränkungen zuerst:** Die Firmware (`muenze8`) ist am 29.10.2026 auf einen StickS3 geflasht worden (esptool, Hashes verifiziert). Ob die App danach startet und funktioniert, ist **noch nicht bestätigt**. Gehäuse nicht gedruckt.
> Am PC geprüft (29.10.2026): Kernlogik in Host-Tests (Münze 8 = 130, Münze 12 = 188, Prisma 10 = 140 Prüfungen), Gehäuse-Passung gegen das M5-Strukturmodell (rechnerisch) und dass alle drei Firmware-Varianten mit der ESP32-S3-Toolchain kompilieren (`make build`). „Kompiliert“ heißt nicht „läuft“.

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
- **Seitentaster** (Reset/Einschalten, Download-Modus, siehe Abschnitt Flashen) sind bei eingebautem Stick **nicht erreichbar**; die Münze hat dafür keine Löcher. Das Zehnkant-Prisma hat Ø5,5-Zugangslöcher.
- **Stabilität (gerechnet, nicht gemessen):** Aufgestellt kippt die Münze ab ca. 21° zur Nachbarseite und ab ca. 20° flach um. Zum Vergleich: die 12-Eck-Münze mit 18,6 mm kippte ab ca. 13°.
- **Ziffern:** 1–8 auf den Randflächen, 6. mit Punkt. Lesbar von vorne-oben, wenn das Display zu einem zeigt.
- **Anpassen:** `python3 muenze_gehaeuse.py --seiten N --dicke D --usb auto|ja|nein --check ../ref/StickS3.stl` (Abhängigkeiten: `pip install manifold3d numpy matplotlib`, geprüft mit manifold3d 3.5.4; das Referenzmodell lädt `make ref`).
- **Entfernt:** Das frühere Sechskant-Prisma (5 Projekte, `zeitprisma_gehaeuse.stl`, `vorschau.png`) ist gelöscht, weil kein Skript und keine Firmware-Variante es noch erzeugt oder nutzt. Es liegt im Git-Verlauf (Commit a231673).

## Firmware (Kurzfassung, Details im Abschnitt „Funktionsweise“)
- **Einrichten:**
  1. WLAN-Zugangsdaten in `firmware/include/config_local.h` eintragen (Vorlage: `config_local.h.example`; die Datei steht in `.gitignore` und kommt nie nach GitHub), Projektnamen in `firmware/include/config.h`.
  2. Flashen: siehe Abschnitt „Flashen“ (`make flash`, bei Problemen Download-Modus).
  3. Beim ersten Start: Uhrzeit per NTP holen, dann Teach-in. Dazu das Display nach oben legen und danach die Seiten 1…8 der Reihe nach nach oben stellen, jeweils ca. 1 s ruhig.
- **Seitenwechsel:** Nach 3 s Ruhe wird die neue Seite übernommen, mit einem Fangbereich von ±18°.
- **Töne:** 5 ist ein langer Ton, der Rest kurze (8 = lang + 3 kurz). Pause ist ein tiefer Ton.
- **Anzeige:** Ist die Münze aufgestellt, dreht sich der Text automatisch aufrecht. Die Richtung muss einmal kalibriert werden (`DISP_ROT_OFFSET/DIR`). Bei 8 Seiten steht der Text auf den schrägen Seiten (1, 3, 5, 7) um 45° schief.
- **Taste A (vorne):**
  - kurz: Tagesübersicht
  - 2 s halten: WLAN-Export unter `http://zeitprisma.local` mit `log.csv`
  - 6 s halten: Teach-in
- **Log:** `zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok`, eine Zeile je Wechsel.
- **Auswertung:** `python3 tools/auswertung.py zeitprisma.csv [--woche] [--csv out.csv]`

## Flashen
- **Normalfall:** Stick wach und am USB (Linux: Port `/dev/ttyACM0`, Benutzer in der Gruppe `dialout`), dann `make flash ENV=muenze8` (= `cd firmware && pio run -e muenze8 -t upload`; `pio` muss im PATH sein). esptool setzt den Stick selbst in den Bootloader und startet ihn danach neu.
- **Wenn kein Port erscheint oder der Stick im Sekundentakt neu startet:** Download-Modus erzwingen. Am 29.10.2026 trat das Neustart-Verhalten auf (im Kernel-Log ständig neue USB-Gerätenummern, Ursache ungeklärt). Laut M5-Doku: USB-Kabel anschließen und die **seitliche Reset-Taste gedrückt halten, bis die interne grüne LED blinkt** (eine Haltedauer nennt die Doku nicht). Danach bleibt `303a:1001` (`lsusb`) dauerhaft stehen, ohne neue Gerätenummern. So hat es hier funktioniert.
- **Prüfen, ohne den Download-Modus zu verlassen:** `esptool.py --chip esp32s3 --port /dev/ttyACM0 --after no_reset flash_id`. Erwartet: ESP32-S3-PICO-1, 8 MB Flash, 8 MB PSRAM, am Ende „Staying in bootloader.“
- **Bootloader oder laufende App?** `esptool.py ... --before no_reset --after no_reset chip_id`: Antwortet der Chip (MAC wird gezeigt), steht er im Bootloader. Die laufende App antwortet nicht.
- **Nach dem Flashen:** Am 29.10.2026 blieb der Stick trotz „Hard resetting via RTS pin“ im Bootloader (esptool antwortete danach ohne Reset). Vermutung: Der Download-Modus ist an der Reset-Taste/dem M5PM1 verriegelt. Dann die Seitentaste **einmal kurz klicken** (M5-Doku: Einfachklick = Einschalten/Reset, Doppelklick = Ausschalten). *Bestätigung am Gerät steht noch aus.*
- **Mitlesen:** `make log` (= `tools/seriallog.py -s 30`, braucht pyserial, z. B. mit dem Python von PlatformIO: `~/.platformio/penv/bin/python tools/seriallog.py -s 30`). Die Startzeile erscheint rund 1 s nach dem Reset, meist bevor der Rechner den Port geöffnet hat. Deshalb den Logger **vor** dem Reset starten.
- **Deep Sleep:** Im Deep Sleep verschwindet der USB-Port (laut `seriallog.py`), zum Flashen den Stick bewegen oder Taste A drücken. Bei eingebautem Stick gibt es keinen Zugang zur Seitentaste.

## Funktionsweise (aus dem Code, Stand 29.10.2026; Konstanten in `firmware/include/config.h`)

### Gespeicherter Zustand
- **RTC-RAM** (überlebt Deep Sleep, nicht Stromverlust, Flashen oder Bauform-Wechsel): aktive Seite, Beginn des Abschnitts, Tagessummen je Projekt, Zeitpunkt des letzten NTP-Syncs, „Zeit gültig“, Zähler unruhiger Sitzungen. Prüfwert `0x5A500300 | Seitenzahl`. Stimmt er nicht, ist es ein **Kaltstart**.
- **NVS** (Namespace `prisma`): Teach-in-Seitenmodell, Version `0x300 | Seitenzahl`. Passt die Version nicht, gilt das Modell als ungültig und es folgt ein Teach-in.
- **LittleFS:** `/log.csv`. Ist das Dateisystem leer oder defekt, wird es formatiert.

### Start
1. Display quer (Rotation 1), M5PM1 und BMI270 initialisieren. Bei Fehler zeigt das Display „Hardwarefehler / PM1/IMU pruefen“ (beim Kaltstart gleich überschrieben), seriell steht „M5PM1 nicht erreichbar“ bzw. „BMI270 nicht gefunden“.
2. Seitenmodell laden, sonst Notlösung `defaultModel()` (ungültig).
3. Serielle Startzeile (siehe „Serielle Ausgabe“).
4. **Kaltstart:** Display „Zeitprisma / hole Uhrzeit ...“, WLAN (max. 8 s) und NTP (max. 6 s), dann bei ungültigem Modell Teach-in, dann Status für 5 s.
5. **Wecken per Taste A:** Status für 8 s. **Wecken per Bewegung:** Display bleibt aus, bis es etwas zu zeigen gibt.

### Seitenerkennung (alle 20 ms)
- **Ruhig** heißt: 25 Samples (0,5 s) weichen alle weniger als 3° vom Mittel ab, und der Betrag liegt zwischen 0,8 und 1,2 g.
- **Seite** = die gelernte Seite, deren Vektor höchstens den Fangbereich (0,4 × Schrittwinkel, bei 8 Seiten ±18°) vom Mittelwert entfernt ist. Sonst „unbekannt“ (−1), das zählt als Pause.
- Eine neue Seite wird erst übernommen, wenn sie **3 s ununterbrochen ruhig** lag. Bewegung setzt den Timer zurück.
- **Übernehmen:** Zeit des vorigen Abschnitts dem alten Projekt gutschreiben (nur bei gültiger Zeit, an Mitternacht geteilt, im Gerät bleibt nur der heutige Tag), CSV-Zeile anhängen, seriell „Seite n -> Name“, Töne, Anzeige (Seite 0: Status 8 s; Randseite: Projektfarbe und Name 2,5 s, aufrecht gedreht), NTP-Sync, wenn der letzte älter als 6 h ist.
- Projektnummer: Seite 0 und „unbekannt“ = Projekt 0 (Pause), Seite k = Projekt k.

### Teach-in
- **Start:** automatisch bei Kaltstart ohne gültiges Modell, sonst Taste A 6 s halten und loslassen.
- **Ablauf:** Display „Teach-in / Display nach oben legen / und ruhig liegen lassen“ und langer Ton. Erst Seite 0 (Display oben, flach), dann nacheinander Seite 1 bis 8 nach oben bringen (Münze auf den Rand stellen). Jede Lage muss **1 s ruhig** liegen. Bestätigung: Ton und „Seite k erfasst / weiter: k+1 nach oben“, am Ende „fertig“.
- **Annahmeregeln** (sonst wird die Lage still ignoriert): mindestens 0,65 × Schrittwinkel (bei 8 Seiten 29°) Abstand zu allen bisherigen. Ab der zweiten Randseite liegt jede etwa einen Schrittwinkel (±40 %, bei 8 Seiten 27° bis 63°) neben der zuletzt erfassten. Randseiten liegen höchstens 30° aus der Randebene, also 60° bis 120° zu Seite 0.
- **Zeitlimit:** 180 s je Seite. Danach „Teach-in / abgebrochen (Zeit)“, drei tiefe Töne, das alte Modell bleibt.
- **Abschlussprüfung:** Nachbarseiten im Ring 45° ± 8°, Randseiten 90° ± 8° zu Seite 0. Sonst „Reihenfolge falsch?“ mit „Abweichung max x Grad“, drei tiefe Töne, altes Modell bleibt. Erfolg: „Teach-in OK / gespeichert“, langer Ton, Modell im NVS.
- Ohne Gehäuse von Hand sind ±8° vermutlich kaum zu treffen. Sinnvoll ist das Teach-in mit dem gedruckten Gehäuse.

### Schlafen und Wecken
- **Wann:** Sobald nichts mehr angezeigt wird, Taste A nicht gedrückt ist und die Seite übernommen und stabil ist. Spätestens 45 s nach Sitzungsbeginn (z. B. in der Tasche), dann zählt die Sitzung als „unruhig“. Nach 3 unruhigen Sitzungen in Folge: 5 min Timer-Schlaf.
- **Deep Sleep:** Display aus, Backlight/Mikrofon/Lautsprecher (L3B) über M5PM1 PYG2 aus, BMI270 any-motion scharf (Schwelle `ANYMOT_THRESHOLD` 0xAA, ca. 80 mg, 100 ms).
- **Wecken:** Bewegung (BMI270 INT1 → M5PM1 PYG4 → IRQ PYG1 → G13, EXT0 low) oder Taste A (G11, EXT1 low). Wird die Bewegungskette nicht scharf, gilt ein Timer-Rückfall von 60 s und seriell steht „Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s“.
- `POWER_DEEPSLEEP 0` schaltet auf Light Sleep um, alle 700 ms wird die Lage geprüft.

### Anzeige und Töne
- **Status** (Seite 0 oder Taste A kurz): „Heute dd.mm. HH:MM“ (oder „Zeit unbekannt“), rechts oben die Akkuspannung, Projekte in zwei Spalten mit Tagessumme h:mm. Das laufende Projekt zählt die laufende Zeit live dazu.
- **Projektbestätigung:** Fläche in Projektfarbe, Name groß, „Seite k“.
- **Töne:** Projekt p ab 1: je 5 ein langer Ton (2200 Hz, 300 ms), der Rest kurze (2600 Hz, 70 ms), also 7 = lang + 2 kurz. Pause = ein tiefer Ton (1500 Hz). Langer Ton (1800 Hz, 350 ms) = Teach-in Start/Ende, WLAN-Export bereit. Drei tiefe Töne (800 Hz) = Fehler oder Abbruch.
- **Rotation:** Status und Texte quer (`DISP_ROT_FLAT` = 1). Münze aufgestellt: `(DISP_ROT_OFFSET + DISP_ROT_DIR × Quadrant) & 3`, der Quadrant ist die Lage in der Displayebene in 90°-Schritten.

### WLAN-Export
Taste A 2 s halten und loslassen: WLAN verbinden (8 s), nebenbei NTP, Webserver auf Port 80 und mDNS `zeitprisma.local`. Das Display zeigt „WLAN-Export / http://zeitprisma.local / IP“, dazu ein langer Ton. Seite `/` zeigt Tagessummen, aktives Projekt, Akku und Log-Größe, `/log.csv` lädt `zeitprisma.csv` herunter. Ende: Taste A klicken oder nach 5 min. Ohne WLAN: „keine Verbindung“ und drei tiefe Töne.

### Log-Datei
`/log.csv` auf LittleFS, Kopf `zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok`, eine Zeile je übernommenem Wechsel. `zeit` ist die lokale ISO-Zeit (leer, wenn die Uhrzeit nicht gültig war, dann `zeit_ok` = 0), `epoch` Sekunden, `seite` −1 bei unbekannter Lage, `projekt` 0 = Pause, `name` ohne Komma und Anführungszeichen, `vbat_mv` Akkuspannung.

### Serielle Ausgabe (USB-CDC, Log-Level Info)
Eigene Ausgaben der Firmware:
- `Start: kalt=<1|0> Weckgrund=<n> Seite=<Seite> Modell=<1|0> Akku=<mV> mV Bauform=Muenze/9`. **Weckgrund** (ESP-IDF): 0 = kein Aufwachen aus Deep Sleep (Einschalten, Reset, Flashen), 2 = Bewegung (G13), 3 = Taste A, 4 = Timer. `Seite` −1 = Pause oder unbekannt. `Muenze/9` = Münze mit 8 Randseiten plus Seite 0, beim Prisma `Prisma/10`.
- `Seite <n> -> <Projektname>` bei jedem übernommenen Wechsel.
- Warnungen und Fehler: `LittleFS leer/defekt -> formatiere`, `WLAN nicht erreichbar`, `Log schreiben fehlgeschlagen`, `M5PM1 nicht erreichbar`, `BMI270 nicht gefunden`, `Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s`.
- Im Teach-in und beim Warten wird nichts ausgegeben. Die WLAN-Zugangsdaten gibt die Firmware nirgends aus. Ob Bibliotheken des Arduino-Cores zusätzlich eigene Zeilen schreiben, ist ungeprüft.

## Fehlersuche

| Beobachtung | Bedeutung und Maßnahme |
|---|---|
| Stick taucht alle 1–2 s am USB auf und verschwindet | Neustart- oder Wecken-Schleife, Ursache offen. Download-Modus erzwingen und neu flashen. |
| Nach dem Flashen bleibt der Port stabil, aber es kommt kein Log und das Display tut nichts | Stick steht noch im Download-Modus. Seitentaste einmal klicken. Kontrolle: `esptool ... --before no_reset chip_id` antwortet danach nicht mehr. |
| Display „Zeit unbekannt“ | WLAN oder NTP fehlgeschlagen. `config_local.h` prüfen (SSID, Passwort, nur 2,4-GHz-Netz), seriell „WLAN nicht erreichbar“. |
| „Hardwarefehler / PM1/IMU pruefen“ | M5PM1 oder BMI270 nicht per I2C erreichbar. I2C nur über `M5.In_I2C` nutzen. |
| Teach-in „abgebrochen (Zeit)“ oder „Reihenfolge falsch?“ | Siehe Teach-in. Taste A 6 s halten startet es neu. |
| Seite wird nicht erkannt oder springt | Lage 3 s ruhig halten, Fangbereich ±18°, ggf. Teach-in neu. |
| Nach dem Flashen kommt sofort wieder das Teach-in | Gewollt: Prüfwert und Modell-Version ändern sich mit der Bauform, das Flashen löscht den RTC-Zustand. |

## Strom
`POWER_DEEPSLEEP 1`: Bewegung weckt den ESP32 über BMI270 any-motion → M5PM1 PYG4 → IRQ PYG1 → G13. Danach wird abgetastet, geloggt und wieder geschlafen. M5 gibt für diesen Board-Zustand (L2) ca. 102 µA an. Rechnerisch reicht das für einige Wochen pro Ladung, real bitte messen: `vbat_mv` wird mitgeloggt. Dauerwackeln (Tasche) → 5 min Timer-Schlaf. Rückfall `POWER_DEEPSLEEP 0`: Light Sleep mit Abfrage, einfacher, aber spürbar mehr Verbrauch.

## Offene Punkte (ungetestet)
- **App-Start:** Ob die geflashte Firmware auf dem Stick anläuft, ist nicht bestätigt (siehe Flashen).
- **Weckkette:** Die BMI270-any-motion-Register sind aus der Bosch-SensorAPI übernommen, die M5PM1-Konfiguration aus der M5-Doku. Beides muss auf der Hardware verifiziert werden.
- **Magnet-Halt:** Annahme „Magnetic back“ hält an Stahl. Der Stick sitzt nur magnetisch in der offenen Tasche, falls zu schwach: dünnes doppelseitiges Klebeband.
- **Display-Rotation:** `DISP_ROT_OFFSET/DIR` in `config.h` einmal am Gerät kalibrieren.
- **Geometrie:** Gehäusemaße aus dem offiziellen Strukturmodell (`M5_Hardware/Products/K150_StickS3`), Schwerpunkt des Sticks unbekannt (Annahme: Mitte), USB-Loch nur für schlanke Stecker.

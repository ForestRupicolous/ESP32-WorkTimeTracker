# Time Roll (Zeitprisma)

Projektzeit-Tracker im Stil von TimeFlip mit M5Stack StickS3, Fortsetzung des Projekts „M5StickC S3 Projektzeitschreiber“.
Entwicklung (Hardware-Fakten mit Quellen, Befehle, Konventionen, nächste Schritte): siehe `CLAUDE.md`.

> **Stand, Einschränkungen zuerst:** Die Firmware (`muenze8`) ist am 29.10.2026 auf einen StickS3 geflasht worden (esptool, Hashes verifiziert) und **startet**: Die Startzeile wurde mitgeschnitten, M5PM1 (Akku 4170 mV gelesen) und BMI270 sind ohne Fehlermeldung initialisiert, das WLAN hat sich offenbar verbunden. Rückmeldung nach dem Test am Gerät (29.10.2026): Der Seitenwechsel soll deutlich schneller erfolgen, und solange der Stick ca. in der Drehachse steht, soll immer ein Projekt erkannt werden, mit Hysterese. Das ist als Ringerkennung umgesetzt (siehe Funktionsweise), am PC getestet und geflasht. Am Gerät im 200-s-Mitschnitt vom 29.10.2026 (Stick ohne Gehäuse von Hand gerollt): 24 Seitenwechsel (Seiten 1, 3, 5, 6, 7, 8 und Pause), **kein einziger „unbekannt“**, die erste Seite stand 1,6 bis 2,4 s nach dem Chip-Start im Log (davon ca. 0,8 s Boot). Danach wurden Boot und Entprellung verkürzt (Ruhefenster 0,12 s, `SETTLE_MS` 250 ms, Warmstart ohne M5-IMU-Init, siehe „Start“) und in einem zweiten 200-s-Mitschnitt gemessen: 26 Seitenwechsel (alle 8 Randseiten und Pause), kein „unbekannt“, kein Wechsel im selben Wachabschnitt unter 1,4 s Abstand, Startzeile 0,57 s nach dem Chip-Start (vorher 0,80 s), erste Seite 1,0 bis 1,3 s nach dem Chip-Start. Wie sich das anfühlt, bewertet der Nutzer. Die Wecken-Schleife (der Stick wachte nach dem Einschlafen sofort wieder auf) ist behoben, Ursache und Fix siehe „Diagnose der Weckkette“. Die Standard-Version ohne Diagnose ist geflasht: Im selben Mitschnitt weckte Bewegung den Stick 4 Mal, und er schlief unberührt 84 s lang ohne ein einziges Aufwachen; im zweiten Mitschnitt weckte sie ihn 8 Mal, unberührt schlief er 35 s und 55 s am Stück. Tasten, Töne, Teach-in mit Gehäuse und Stromverbrauch sind nicht getestet. Gehäuse nicht gedruckt.
> Am PC geprüft (29.10.2026): Kernlogik in Host-Tests (Münze 8 = 236, Münze 12 = 326, Prisma 10 = 255 Prüfungen), Gehäuse-Passung gegen das M5-Strukturmodell (rechnerisch) und dass alle drei Firmware-Varianten mit der ESP32-S3-Toolchain kompilieren (`make build`). „Kompiliert“ heißt nicht „läuft“.

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
- **Seitenwechsel:** Steht der Stick ca. in der Drehachse (Münze auf dem Rand), wird immer die nächste Seite erkannt, auch zwischen zwei Seiten, mit Hysterese an den Seitengrenzen. Die neue Seite wird rund 0,4 s nach dem Stillstand übernommen (0,12 s Ruhefenster plus 0,25 s).
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
- **Nach dem Flashen die App starten:** Der „Hard resetting via RTS pin“ von esptool lässt den Stick hier im Download-Modus stehen. Am 29.10.2026 gemessen: Nach einem RTS-Reset meldet das ROM `rst:0x15 (USB_UART_CHIP_RESET),boot:0x21 (DOWNLOAD(USB/UART0))` und „waiting for download“, die App startet nicht. **Ein Watchdog-Reset startet die App:** `esptool.py --chip esp32s3 --port /dev/ttyACM0 --before default_reset --after watchdog_reset chip_id` (esptool liegt unter `~/.platformio/packages/tool-esptoolpy/`; der USB-Port meldet sich dabei neu an, die Port-Nummer kann sich ändern). Damit wurde die Startzeile gesehen. Ein Klick auf die Seitentaste wurde nicht gezielt getestet (M5-Doku: Einfachklick = Einschalten/Reset, Doppelklick = Ausschalten).
- **ModemManager (Linux):** Er hielt am 29.10.2026 den Port des Sticks offen (`fuser`), und der Port wanderte von `ttyACM0` auf `ttyACM1`. ModemManager ist ein bekannter Störer bei ESP32-Geräten mit USB (öffnet Ports, setzt DTR/RTS, schickt AT-Befehle). PlatformIO meldet beim Upload außerdem, dass `/etc/udev/rules.d/99-platformio-udev.rules` veraltet ist. Vermutlich hilft es, die Regeln zu erneuern: `curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules`, danach `sudo udevadm control --reload-rules && sudo udevadm trigger` und den Stick neu anstecken (Befehle laut PlatformIO-Doku). Ob das den hängenden Download-Modus nach dem Flashen und das Wandern der Port-Nummer behebt, ist **ungeprüft**. Die Neustart-Schleife erklärt es vermutlich nicht (siehe Offene Punkte, Wecken-Schleife).
- **Mitlesen:** `make log` (= `tools/seriallog.py -s 30`, braucht pyserial, z. B. mit dem Python von PlatformIO: `~/.platformio/penv/bin/python tools/seriallog.py -s 30`). Die Startzeile erscheint rund 1 s nach dem Reset, meist bevor der Rechner den Port geöffnet hat. Deshalb den Logger **vor** dem Reset starten.
- **Deep Sleep:** Im Deep Sleep verschwindet der USB-Port (laut `seriallog.py`), zum Flashen den Stick bewegen oder Taste A drücken. Bei eingebautem Stick gibt es keinen Zugang zur Seitentaste.
- **Schlafenden Stick flashen (am 29.10.2026 so gemacht):** Der Port ist nur 1 bis 3 s nach einer Bewegung da. Eine Schleife prüft alle 50 ms, ob `/dev/ttyACM*` erscheint, und startet dann sofort `esptool.py --chip esp32s3 --port <Port> --before default_reset --after no_reset --connect-attempts 1 flash_id` (mit `timeout 15`). Damit steht der Chip im ROM-Bootloader und bleibt dort stehen. Danach `make flash ENV=muenze8` und der Watchdog-Reset aus dem Punkt „Nach dem Flashen die App starten“. Der Stick muss während der Schleife bewegt werden (Rollen genügt). Ein festes Zeitfenster ohne Absprache scheiterte, weil niemand den Stick bewegte.

## Funktionsweise (aus dem Code, Stand 29.10.2026; Konstanten in `firmware/include/config.h`)
Alles hier ist aus dem Code gelesen. Am Gerät bestätigt sind der Start, das Wecken per Bewegung und die Seitenerkennung beim Rollen von Hand (siehe Stand oben), nicht Teach-in mit Gehäuse, Tasten, Töne und Stromverbrauch.

### Gespeicherter Zustand
- **RTC-RAM** (überlebt Deep Sleep, nicht Stromverlust, Flashen oder Bauform-Wechsel): aktive Seite, Beginn des Abschnitts, Tagessummen je Projekt, Zeitpunkt des letzten NTP-Syncs, „Zeit gültig“, Zähler unruhiger Sitzungen. Prüfwert `0x5A500300 | Seitenzahl`. Stimmt er nicht, ist es ein **Kaltstart**. Dazu der Merker `imuFastOk` in `hal.cpp` (Warmstart ohne M5-IMU-Init erlaubt, siehe „Start“).
- **NVS** (Namespace `prisma`): Teach-in-Seitenmodell, Version `0x300 | Seitenzahl`. Passt die Version nicht, gilt das Modell als ungültig und es folgt ein Teach-in.
- **LittleFS:** `/log.csv`. Ist das Dateisystem leer oder defekt, wird es formatiert.

### Start
1. Display quer (Rotation 1), M5PM1 und BMI270 initialisieren. Bei Fehler zeigt das Display „Hardwarefehler / PM1/IMU pruefen“ (beim Kaltstart gleich überschrieben), seriell steht „M5PM1 nicht erreichbar“ bzw. „BMI270 nicht gefunden“.
   - **Schneller Warmstart (nur nach dem Wecken aus dem Deep Sleep):** M5Unified initialisiert das BMI270 bei jedem `M5.begin()` neu (Reset, 8-KB-Konfiguration hochladen, `INT_MAP_DATA` = 0xFF schreiben, aus `BMI270_Class.inl` gelesen). Nach dem Deep Sleep ist das BMI270 aber noch konfiguriert. Ist der RTC-Merker `imuFastOk` gesetzt, läuft `M5.begin()` deshalb mit `internal_imu = false`, und `hal` liest das BMI270 selbst (Rohwerte 0x0C bis 0x11, Achsen `{Y, −X, Z}` wie M5Unified, Messbereich aus `ACC_RANGE`). `hal::begin()` prüft dabei `INTERNAL_STATUS` (Init ok = 1) und holt sonst die M5-Initialisierung nach (Warnung „BMI270 nicht mehr initialisiert, M5-Init nachgeholt“). Im Warmstart ist `M5.Imu` nicht gestartet: Code außerhalb von `hal.cpp` darf es nicht direkt aufrufen, nur `hal::readAccel()`.
   - **Abgleich, der den Merker setzt** (`imuSelfCheck()`, bei jedem Start **mit** M5-IMU-Init, also Kaltstart und Warmstart ohne Merker): Die eigene Lesung und `M5.Imu.getAccel` dürfen höchstens 0,08 g auseinanderliegen, und beide müssen eine Schwerkraft von 0,8 bis 1,2 g zeigen (sonst könnten zwei Nullvektoren „übereinstimmen“). Das sichert ab, dass das angelernte Seitenmodell zum selben Koordinatensystem gehört. Liegt der Stick beim Start nicht ruhig, fällt der Abgleich durch, und der nächste Start prüft erneut. Ergebnis seriell: `IMU-Abgleich: M5 (x y z) eigen (x y z) max. Abweichung .. g, |g| ../.. -> schneller Warmstart erlaubt|NICHT erlaubt`.
   - **Gemessen (29.10.2026):** Startzeile 0,57 s nach dem Chip-Start mit eigenem Lesen (`begin` = 434 ms), 0,82 s mit M5-IMU-Init (`begin` = 664 ms), beim Kaltstart 1,0 s. Vorher (immer mit M5-Init) 0,80 bis 0,81 s, also spart der Schnellstart ca. 230 ms. Der Abgleich stimmte bei zwei Lagen auf 0,000 g überein (der erste Start wurde wegen 1,62 g Bewegung abgelehnt, der zweite bestand, ab dem dritten lief `imu=eigen`). Der Rest der Bootzeit ist nicht weiter aufgeschlüsselt (siehe die Felder der Startzeile unter „Serielle Ausgabe“). Die Zeit von Reset bis Chip-Start (ROM, Bootloader) steckt in diesen Zahlen nicht drin und ist ungemessen.
2. Seitenmodell laden, sonst Notlösung `defaultModel()` (ungültig).
3. Serielle Startzeile (siehe „Serielle Ausgabe“).
4. **Kaltstart:** Display „Zeitprisma / hole Uhrzeit ...“, WLAN (max. 8 s) und NTP (max. 6 s), dann bei ungültigem Modell Teach-in, dann Status für 5 s.
5. **Wecken per Taste A:** Status für 8 s. **Wecken per Bewegung:** Display bleibt aus, bis es etwas zu zeigen gibt.

### Seitenerkennung (alle 20 ms)
- **Ruhig** heißt: 6 Samples (0,12 s, `STILL_WINDOW`) weichen alle weniger als 3° vom Mittel ab, und der Betrag liegt zwischen 0,8 und 1,2 g. Nur dann wird die Seite bestimmt, sonst bleibt der bisherige Kandidat.
- **Ringerkennung ohne Totzone** (`classifyRing()` in `prisma_core`): Liegt der Beschleunigungsvektor ca. in der Ringebene („Stick in der Drehachse“, Rollachse waagrecht), wird **immer** die nächste Randseite geliefert, auch zwischen zwei Seiten. Die Ringebene steht senkrecht zur Drehachse (Münze: Display-Normale, Prisma: Längsachse). Verglichen wird der Winkel in der Ringebene, ein Kippen aus der Ebene verfälscht ihn nicht.
- **Hysterese**, jeweils bezogen auf die bisherige Seite (der aktuelle Kandidat):
  - *Nachbarseiten:* Gewechselt wird erst, wenn eine andere Seite um mehr als `RING_HYST_DEG` näher liegt (20 % des Seitenabstands, bei 8 Seiten 9°). Bei 8 Seiten schaltet es von Seite i zu i+1 bei 27° Abstand zur Mitte von i, zurück erst bei 18°. Dazwischen bleibt die bisherige Seite.
  - *Ringebene:* Eintritt bei höchstens 35° Kippung aus der Ebene (`RING_UPRIGHT_DEG`), Verbleib bis 45° (`RING_UPRIGHT_EXIT_DEG`).
  - *Münze auf dem Rücken (Seite 0):* Eintritt bei höchstens 30° von „Display oben“ (`FLAT_DEG`), Verbleib bis 40° (`FLAT_EXIT_DEG`).
- **Außerhalb** davon ist die Lage „unbekannt“ (−1) und zählt als Pause: Münze mit Display unten oder stärker schräg, Prisma aufrecht stehend. Beim Prisma gehört Seite 0 zum Ring.
- Eine neue Seite wird erst übernommen, wenn sie **`SETTLE_MS` = 0,25 s** ununterbrochen ruhig lag, nach dem 0,12 s Ruhefenster also rund 0,4 s nach dem Stillstand (Abtasttakt 20 ms). Bewegung setzt den Timer zurück. Nach dem Wecken aus dem Deep Sleep kommt der Start dazu (die Startzeile erscheint 0,57 s nach dem Chip-Start, siehe „Start“). Gemessen: Der erste Wechsel stand 1,0 bis 1,3 s nach dem Chip-Start im Log, davon rund 0,4 s Erkennung, wenn der Stick sofort ruhig lag.
- Kürzere Zeiten erkennen schneller, aber Zwischenseiten beim schnellen Rollen werden eher mitgezählt: Wer eine Seite länger als `STILL_WINDOW` + `SETTLE_MS` ruhig lässt, bekommt sie als Wechsel. Wurden beim Rollen falsche Zwischenseiten gezählt, `SETTLE_MS` erhöhen. Das war im Mitschnitt vom 29.10.2026 nicht zu sehen (die Reihenfolge der Seiten war lückenlos), schnelleres Rollen ist aber nicht geprüft.
- Frühere Versionen: Zuerst Fangbereich ±18° um jede Seite mit 9° Totzone dazwischen (= Pause), 0,5 s Ruhefenster und 3 s Entprellung, zusammen ca. 3,5 s. Danach (Ringerkennung) 0,2 s Ruhefenster und 0,6 s Entprellung, zusammen ca. 0,8 s, erste Seite 1,6 bis 2,4 s nach dem Chip-Start.
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
- **Deep Sleep, Ablauf:** Display aus, Backlight/Mikrofon/Lautsprecher (L3B) über M5PM1 PYG2 aus. Das BMI270 wird für any-motion konfiguriert (Schwelle `ANYMOT_THRESHOLD` 0xAA, ca. 80 mg, Dauer 5 Samples = 100 ms) und darf **nur INT1** treiben: `INT_MAP_DATA`, `INT2_MAP_FEAT` und `INT2_IO_CTRL` werden auf 0 geschrieben (siehe Diagnose der Weckkette). Dann wird der M5PM1 scharf gestellt (nur GPIO4 entmaskiert, PYG1 als IRQ-Ausgang). Danach bleibt der Stick wach, löscht den IRQ bei jeder Auslösung und wartet, bis G13 `IRQ_QUIET_MS` = 300 ms ruhig High war (höchstens `IRQ_SETTLE_MAX_MS` = 2,5 s). Erst dann beginnt der Deep Sleep.
- **Wecken:** Bewegung (BMI270 INT1 → M5PM1 PYG4 → IRQ PYG1 → G13, EXT0 low) oder Taste A (G11, EXT1 low). Kommt G13 nicht zur Ruhe (echte Dauerbewegung, z. B. in der Tasche) oder wird die Kette nicht scharf, gilt ein Timer-Rückfall von 60 s und seriell steht „Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s“.
- `POWER_DEEPSLEEP 0` schaltet auf Light Sleep um, alle 700 ms wird die Lage geprüft.

### Diagnose der Weckkette (`WAKE_DIAG` in `config.h`)
Zur Fehlersuche (kostet Strom, danach auf 0). Beim Wecken werden die Quellen gelesen, **bevor** die Firmware sie löscht, und vor dem Einschlafen werden sie ausgegeben (sonst gehen die Zeilen beim USB-Neuaufbau verloren). Nach dem Armieren liest der Stick die BMI270-Konfiguration zurück, verfolgt 1,5 s lang INT1 und G13 mit Mikrosekunden-Zeitstempeln (ohne zu löschen) und bleibt danach noch 1,5 s wach, um die Bewegung in mg zu loggen.
- `DIAG beim Wecken (Grund n): G13=x PM1 irq g=.. s=.. b=.. in1=x in4=x | BMI int0=.. int1=..`
  - `G13`: Pegel der Weckleitung (M5PM1-IRQ-Ausgang PYG1), 0 = Low = Wecken.
  - `g`, `s`, `b`: M5PM1-IRQ-Status (Hex) für GPIO, System und Taste. `g` Bit 4 (`10`) = Flanke an PYG4 (BMI270 INT1), `s` Bit 0/1 = 5VIN eingesteckt/abgezogen, Bit 4/5 = Akku eingesteckt/entfernt, `b` Bit 0/1/2 = Klick/Wecken/Doppelklick. Bitwerte laut `M5PM1.h`.
  - `in1`, `in4`: Eingangspegel von PYG1 (IRQ-Ausgang) und PYG4 (BMI270 INT1). INT1 ist low-aktiv, im Ruhezustand also 1.
  - `int0`, `int1`: BMI270 `INT_STATUS_0/1`. Bit 6 von `int0` (`40`) = any-motion.
- `DIAG BMI: ACC_CONF=.. ACC_RANGE=.. INT1_IO=.. INT_LATCH=.. INT1_MAP_FEAT=.. INT_MAP_DATA=.. PWR_CONF=.. PWR_CTRL=.. INTERNAL_STATUS=..` und `DIAG BMI any-motion: w0=.. w1=..`: Register, wie sie im Chip stehen. Soll: `ACC_CONF=17`, `INT1_IO=08`, `INT1_MAP_FEAT=40`, **`INT_MAP_DATA=00`**, `PWR_CONF=01`, `PWR_CTRL=04`. Any-motion: `w0=E005` (Dauer 5, alle Achsen), `w1=B8AA` (Schwelle 0xAA, aktiviert). Die Wörter werden bei kurz ausgeschaltetem Advanced-Power-Save gelesen, sonst kommen Nullen.
- `DIAG Trace: n PM1-Lesungen in 1500 ms, k Ereignisse, G13 am Ende x`, danach je Ereignis `DIAG <µs> us: in4=x` (INT1, über den M5PM1 gelesen) oder `G13=x`. Im Soll nur das erste `in4=1` und G13 am Ende 1.
- `DIAG Ausloesung n nach x ms, IRQ geloescht` (die ersten drei), dann `DIAG ruhig nach x ms, n Ausloesung(en)` oder `DIAG nicht zur Ruhe gekommen: n Ausloesungen in 2500 ms`.
- `DIAG armiert: G13=x`, bei jeder Änderung von G13 `DIAG t=<ms>: ...` mit den Statusfeldern, am Ende `DIAG Ende nach 1500 ms: G13=x, Hub in mg: x=.. y=.. z=..`. Der Hub (Spanne aus Min/Max der BMI270-Rohwerte) zeigt, ob der Stick wirklich wackelt. Die any-motion-Schwelle liegt bei ca. 80 mg.

**Befund (29.10.2026): Ursache der Wecken-Schleife.** Nach `M5.begin()` stand `INT_MAP_DATA` (0x58) des BMI270 auf `0xFF`. Das legt Daten-bereit-, FIFO- und Fehler-Interrupts zusätzlich auf INT1 und INT2. INT1 pulste dadurch im 50-Hz-Takt des Sensors, der M5PM1 merkte sich die Flanken (`g=10`, GPIO4) und zog G13 auf Low, sodass der Stick direkt nach dem Einschlafen wieder geweckt wurde. Im Trace erschienen die Impulse nur auf einem Raster von ca. 20,1 ms (Abstände von 4, 7, 8, 10, 11 oder 15 Samples), weil das 1,3-ms-Leseraster die kurzen Impulse nur zufällig trifft. Auch das frühere Löschen des IRQ nach 30 ms half nicht. Die any-motion-Konfiguration selbst war immer richtig. **Fix:** beim Armieren `INT_MAP_DATA`, `INT2_MAP_FEAT` und `INT2_IO_CTRL` auf 0 schreiben und vor dem Einschlafen warten, bis G13 300 ms ruhig High ist. Danach: keine INT1-Impulse mehr im Trace, `ruhig nach 300 ms, 0 Ausloesung(en)`, G13 blieb High, der Stick blieb im Deep Sleep. Der Hub lag dabei bei ruhig liegendem Stick unter 40 mg, deutlich unter der any-motion-Schwelle von ca. 80 mg.

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
- `Start: kalt=<1|0> Weckgrund=<n> Seite=<Seite> Modell=<1|0> Akku=<mV> mV Bauform=Muenze/9 | Boot ms: setup@<ms> begin=<ms> ui=<ms> hal=<ms> store=<ms> imu=<eigen|M5>`. **Weckgrund** (ESP-IDF): 0 = kein Aufwachen aus Deep Sleep (Einschalten, Reset, Flashen), 2 = Bewegung (G13), 3 = Taste A, 4 = Timer. `Seite` −1 = Pause oder unbekannt. `Muenze/9` = Münze mit 8 Randseiten plus Seite 0, beim Prisma `Prisma/10`. **Boot ms** (in ms, `millis()`): `setup@` = Chip-Start bis `setup()`, `begin` = `M5.begin()`, `ui` = `ui::begin()`, `hal` = `hal::begin()` (bei M5-IMU-Init mit Abgleich), `store` = LittleFS und Modell laden, `imu` = `eigen` (Warmstart ohne M5-IMU-Init) oder `M5`. Der Zeitstempel vorn in der Zeile ist genau diese Summe. Gemessene Werte: Warmstart `setup@64 begin=434 ui=0 hal=72 store=2..6 imu=eigen` (Summe 572..576), mit M5-Init `begin=664 hal=89 store=5 imu=M5` (822), Kaltstart `setup@247 begin=664 ui=1 hal=89 store=5` (1006).
- `IMU-Abgleich: ...` bei jedem Start mit M5-IMU-Init (siehe „Start“). Warnung `BMI270 nicht mehr initialisiert, M5-Init nachgeholt`, wenn der Schnellstart das BMI270 nicht initialisiert vorfand.
- `Seite <n> -> <Projektname>` bei jedem übernommenen Wechsel.
- Warnungen und Fehler: `LittleFS leer/defekt -> formatiere`, `WLAN nicht erreichbar`, `Log schreiben fehlgeschlagen`, `M5PM1 nicht erreichbar`, `BMI270 nicht gefunden`, `Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s`.
- Im Teach-in und beim Warten wird nichts ausgegeben. Die WLAN-Zugangsdaten gibt die Firmware nirgends aus.
- Zeilenformat: `[ms seit Start][Level][Datei:Zeile] Funktion(): Text`. Am Gerät gesehen (29.10.2026, nach Watchdog-Reset): dreimal `i2cInit(): Initialising I2C Master: sda=47 scl=48 freq=100000` (Arduino-Core), dann `setup(): Start: kalt=1 Weckgrund=0 Seite=-1 Modell=0 Akku=4170 mV Bauform=Muenze/9`, nach rund 6 s `WiFiGeneric.cpp ... Reason: 8 - ASSOC_LEAVE` (der Core meldet das eigene Abmelden nach dem NTP-Sync, kein Fehler). Fehlt „WLAN nicht erreichbar“, hat sich das WLAN verbunden. Ob NTP die Uhrzeit geliefert hat, steht nicht im Log (das Display zeigt „Heute …“ oder „Zeit unbekannt“).
- Nach dem Flashen und nach einem Watchdog-Reset ist `kalt=1` und `Modell=0`, es folgt also das Teach-in.

## Fehlersuche

| Beobachtung | Bedeutung und Maßnahme |
|---|---|
| Stick taucht alle 1–2 s am USB auf und verschwindet | Wecken-Schleife (Deep Sleep, sofort wieder geweckt). Ursache am 29.10.2026 gefunden und behoben (BMI270 `INT_MAP_DATA`, siehe Diagnose der Weckkette). Kommt es wieder: `WAKE_DIAG 1` setzen und `DIAG Trace` auswerten. Zum Flashen Download-Modus erzwingen. |
| Nach dem Flashen bleibt der Port stabil, aber es kommt kein Log und das Display tut nichts | Stick steht noch im Download-Modus (ROM meldet `boot:0x21`). Watchdog-Reset auslösen (siehe Flashen). Kontrolle: `esptool ... --before no_reset chip_id` antwortet danach nicht mehr. |
| Port wechselt von `ttyACM0` auf `ttyACM1`, esptool meldet „Write timeout“ oder „port busy“ | Ein anderer Prozess hält den alten Port offen, meist ModemManager (`fuser -v /dev/ttyACM*`). Siehe ModemManager im Abschnitt Flashen. „Write timeout“ ohne Bootloader deutet darauf hin, dass die App läuft und nichts liest (so einmal beobachtet). |
| Display „Zeit unbekannt“ | WLAN oder NTP fehlgeschlagen. `config_local.h` prüfen (SSID, Passwort, nur 2,4-GHz-Netz), seriell „WLAN nicht erreichbar“. |
| „Hardwarefehler / PM1/IMU pruefen“ | M5PM1 oder BMI270 nicht per I2C erreichbar. I2C nur über `M5.In_I2C` nutzen. |
| Teach-in „abgebrochen (Zeit)“ oder „Reihenfolge falsch?“ | Siehe Teach-in. Taste A 6 s halten startet es neu. |
| Seite wird nicht erkannt, springt oder kommt zu spät | Der Stick muss ca. in der Drehachse stehen (bis 35° aus der Ebene gekippt). Die Erkennung dauert rund 0,4 s nach dem Stillstand (`STILL_WINDOW` + `SETTLE_MS`), nach dem Wecken aus dem Deep Sleep kommt der Start (0,57 s bis zur Startzeile, dazu ROM und Bootloader, ungemessen) dazu. Springt es an einer Seitengrenze: `RING_HYST_DEG` erhöhen. Werden beim Rollen Zwischenseiten mitgezählt: `SETTLE_MS` erhöhen (vorher 600). Sonst Teach-in neu. |
| `IMU-Abgleich: ... NICHT erlaubt` im Log | Der Stick lag beim Start nicht ruhig (|g| außerhalb 0,8 bis 1,2) oder die Achsen stimmen nicht mit M5Unified überein (Abweichung ≥ 0,08 g). Bei Bewegung unkritisch: der nächste Start prüft erneut und bleibt bis dahin beim M5-Init (`imu=M5`). Bleibt es bei ruhigem Stick `NICHT erlaubt`, passt die Achsenabbildung in `readBmiAccel()` nicht, dann den Schnellstart in `main.cpp` abschalten (`ownImu = false`). |
| `E (n) I2S: i2s_driver_uninstall(2048): I2S port 0 has not installed` beim Einschlafen | Harmlos. `hal.cpp` ruft vor dem Schlafen `M5.Speaker.end()`, auch wenn in diesem Wachabschnitt kein Ton lief und der Lautsprecher nie gestartet wurde. Erscheint nur in Wachabschnitten ohne Wechsel. |
| Nach dem Flashen kommt sofort wieder das Teach-in | Gewollt: Prüfwert und Modell-Version ändern sich mit der Bauform, das Flashen löscht den RTC-Zustand. |

## Strom
`POWER_DEEPSLEEP 1`: Bewegung weckt den ESP32 über BMI270 any-motion → M5PM1 PYG4 → IRQ PYG1 → G13. Danach wird abgetastet, geloggt und wieder geschlafen. M5 gibt für diesen Board-Zustand (L2) ca. 102 µA an. Rechnerisch reicht das für einige Wochen pro Ladung, real bitte messen: `vbat_mv` wird mitgeloggt. Dauerwackeln (Tasche) → 5 min Timer-Schlaf. Rückfall `POWER_DEEPSLEEP 0`: Light Sleep mit Abfrage, einfacher, aber spürbar mehr Verbrauch.

## Offene Punkte (ungetestet)
- **Am Gerät nicht getestet:** Teach-in mit Gehäuse, Tasten (auch das Wecken per Taste A), Töne, Anzeige-Rotation, Stromverbrauch, schnelles Rollen mit der kurzen Entprellung. Bestätigt sind Start, Wecken per Bewegung und die Seitenerkennung beim Rollen von Hand (siehe Stand).
- **Bootzeit, Rest (nicht untersucht):** Nach dem Schnellstart bleiben 0,57 s bis zur Startzeile: `setup@64`, `begin` 434 ms (`M5.begin()` ohne IMU: Display- und Power-Initialisierung, nicht aufgeschlüsselt), `hal` 72 ms. Ob sich die Display-Initialisierung beim Wecken per Bewegung sparen lässt (das Display bleibt dort ohnehin aus), ist offen.
- **Wecken per Bewegung (am Gerät bestätigt, 29.10.2026):** Die Wecken-Schleife (nach dem Einschlafen sofort wieder geweckt, dreimal, dann 60 s Timer-Rückfall) ist behoben, siehe „Diagnose der Weckkette“. Mit der Standard-Version (`WAKE_DIAG 0`) weckte Bewegung den Stick im 200-s-Mitschnitt 4 Mal, dazwischen schlief er einmal 84 s unberührt ohne Aufwachen. Wahrscheinlich war das auch die Neustart-Schleife, die der Stick vor dem Flashen zeigte (die alte Firmware hatte denselben Schlafcode, nachgestellt wurde sie nicht). Nicht untersucht: wie viele Wecken ein Dauerrollen auslöst und wie sich Bewegung in einer Tasche verhält (45-s-Sitzungsgrenze, dann 5 min Schlaf).
- **Weckkette, Empfindlichkeit:** Die any-motion-Konfiguration ist am Gerät zurückgelesen und stimmt (Dauer 5, Schwelle 0xAA ca. 80 mg, alle Achsen, nur INT1), und echte Bewegung weckt. Ob die Schwelle für sanftes Drehen der Münze reicht und ob Tischerschütterungen ungewollt wecken, ist nach Gefühl im Alltag abzustimmen (`ANYMOT_THRESHOLD`, `ANYMOT_DURATION`).
- **Magnet-Halt:** Annahme „Magnetic back“ hält an Stahl. Der Stick sitzt nur magnetisch in der offenen Tasche, falls zu schwach: dünnes doppelseitiges Klebeband.
- **Display-Rotation:** `DISP_ROT_OFFSET/DIR` in `config.h` einmal am Gerät kalibrieren.
- **Geometrie:** Gehäusemaße aus dem offiziellen Strukturmodell (`M5_Hardware/Products/K150_StickS3`), Schwerpunkt des Sticks unbekannt (Annahme: Mitte), USB-Loch nur für schlanke Stecker.

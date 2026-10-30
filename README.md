# Time Roll (Zeitprisma)

Projektzeit-Tracker im Stil von TimeFlip: eine flache, 8-eckige Münze, auf deren oben liegender Randseite das laufende Projekt eingestellt wird. Der Stick in der Münze merkt sich, wie lange welches Projekt lief.

**Teil 1** ist die Anleitung zur Bedienung, **Teil 2** beschreibt das Projekt (Aufbau, Gehäuse, Flashen, Stand der Prüfung, Technik).

## Teil 1: Anleitung

> **Voraussetzungen:** Der Stick meldet sich beim Start, beim Datenabruf und gelegentlich bei einem Projektwechsel (um die Uhr nachzustellen) in einem 2,4-GHz-WLAN an und holt dort die Uhrzeit. Name und Passwort des WLAN sind fest im Stick eingestellt (ändern: Teil 2, „Einstellungen“). Ohne gültige Uhrzeit zählt der Stick keine Zeiten. Für den Datenabruf braucht man einen Rechner im selben WLAN.

### Das Prinzip
Der Stick liegt flach in der Mitte der Münze, das Display zeigt nach vorne. Die acht Randseiten sind mit 1 bis 8 beschriftet, jede steht für ein Projekt (Standardnamen „Projekt 1“ … „Projekt 8“, dazu „Pause“; die Namen sind ebenfalls fest im Stick eingestellt). Wie die Münze liegt, sagt dem Stick, woran gearbeitet wird:

| Lage der Münze | Bedeutung |
|---|---|
| Auf dem Rand stehend, Randseite 1–8 **oben** | Projekt 1–8 läuft |
| Auf dem Rücken liegend (Display oben) | Pause |
| Display unten oder unklare Lage | ebenfalls Pause |

Wechselt die oben liegende Seite, schreibt der Stick die Zeit seit dem letzten Wechsel dem bisherigen Projekt gut, hängt eine Zeile an die Log-Datei des Tages an, spielt einen Ton und zeigt das neue Projekt an. Zwischendurch schläft er; Bewegung oder Taste A weckt ihn wieder.

### Erster Start und Einlernen (Teach-in)
Beim ersten Start holt der Stick die Uhrzeit (Display „hole Uhrzeit ...“, bis zu 14 s). Kennt er die Seiten der Münze noch nicht, startet danach das Einlernen. Dabei lernt er, wie „oben“ für jede Seite aussieht:
1. Die Münze mit dem Display nach oben flach hinlegen und ruhig liegen lassen.
2. Nacheinander die Seiten 1 bis 8 nach oben stellen, jede Lage 1 s ruhig.
3. Jede erfasste Seite bestätigt ein Ton und das Display („Seite k erfasst“). Am Ende zeigt das Display „Teach-in OK / gespeichert“, dazu ein langer Ton.

Eine Lage, die nicht passt (zum Beispiel dieselbe Seite noch einmal), ignoriert der Stick ohne Meldung. Läuft das Einlernen schief („Reihenfolge falsch?“ oder 180 s ohne neue Lage), bleibt das alte Ergebnis erhalten (beim ersten Mal also keines). Neu starten: Taste A 6 s halten und loslassen (noch nicht am Gerät ausprobiert). Das Ergebnis bleibt im Stick gespeichert, auch ohne Strom.

### Im Alltag
- **Projekt wechseln:** Die Münze rollen oder drehen, bis die gewünschte Seite oben liegt. Dabei muss der Stick ungefähr in der Drehachse stehen (Münze auf dem Rand, höchstens 35° aus der Ebene gekippt). Eine neue Seite gilt, sobald sie rund 0,4 s ruhig liegt. Zwischen zwei Seiten gibt es keine Lücke: Es gilt immer die nächste Seite, und direkt an einer Seitengrenze bleibt die bisherige gültig, bis die neue deutlich näher liegt (9°). Seiten, die beim Rollen nur kurz durchlaufen werden (weniger als ca. 0,4 s ruhig), zählen nicht.
- **Rückmeldung nach einem Wechsel:** Ein weicher Einzelton und 5 s lang das Display mit Projektfarbe, Name und darunter groß der heute aufsummierten Zeit dieses Projekts (h:mm, `--:--`, solange die Uhrzeit unbekannt ist). Die gerade beginnende Strecke zählt noch nicht mit. Jedes Projekt hat seine eigene Note (aufsteigende Tonleiter: Projekt 1 = 523 Hz bis Projekt 8 = 1319 Hz), die Pause die tiefste (392 Hz). Steht die Münze auf dem Rand, dreht sich der Text aufrecht (siehe „Bekannte Einschränkungen“).
- **Tagesübersicht:** Sie erscheint 8 s lang, wenn die Münze auf den Rücken gelegt wird, bei kurzem Druck auf Taste A und beim kräftigen Schütteln der flach liegenden Münze (vier Ausschläge innerhalb 1 s). Sie zeigt Datum und Uhrzeit, die Akkuspannung und alle Projekte mit der Summe von heute (h:mm); das laufende Projekt zählt live mit.
- **Neuer Tag:** Die Summen auf dem Display beginnen jeden Tag um Mitternacht bei 0; die Log-Dateien bleiben erhalten.
- **Schlafen:** Sobald nichts mehr angezeigt wird und die Seite übernommen ist, schläft der Stick ein. Bewegung oder Taste A weckt ihn wieder. Beim Wecken durch Bewegung bleibt das Display aus, bis es etwas zu zeigen gibt (neue Seite oder Schütteln der liegenden Münze). Die Uhr läuft im Schlaf weiter. Bei Dauerbewegung (z. B. in der Tasche) schläft der Stick spätestens nach 45 s ein, nach drei solchen Sitzungen in Folge 5 min am Stück.

### Taste A (vorne)
| Bedienung | Wirkung |
|---|---|
| kurz drücken | Tagesübersicht (weckt den Stick auch aus dem Schlaf; die Übersicht erscheint nach dem Start, also frühestens ca. 0,6 s nach dem Druck, nicht gemessen) |
| 2 s halten, dann loslassen | Datenabruf per WLAN (das Display zeigt ab 2 s „Loslassen: WLAN-Export“) |
| 6 s halten, dann loslassen | Einlernen neu starten (das Display zeigt ab 6 s „Loslassen: Teach-in“) |

### Daten abrufen und auswerten
- **Auf dem Stick** liegt je Tag eine CSV-Datei mit einer Zeile je Wechsel: `zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok` (`zeit_ok` = 1, wenn die Uhrzeit gültig war). Wechsel ohne gültige Uhrzeit stehen in einer eigenen Datei.
- **Abrufen:** Taste A 2 s halten und loslassen. Der Stick verbindet sich mit dem WLAN und zeigt `http://zeitprisma.local` und seine IP, dazu ein langer Ton. Am Rechner im selben WLAN diese Adresse im Browser öffnen: Die Startseite zeigt die Tagessummen und die Liste der Tagesdateien, „Alle Tage (log.csv)“ lädt alles als `zeitprisma.csv`. Auf der Kommandozeile geht es mit `curl -o zeitprisma.csv http://zeitprisma.local/log.csv` (alle Tage) oder `curl 'http://zeitprisma.local/tag?d=2026-10-29'` (ein Tag). Löst `zeitprisma.local` nicht auf, die IP vom Display nehmen. Beenden: Taste A klicken oder 5 min warten.
- **Auswerten:** `python3 tools/auswertung.py zeitprisma.csv [--woche] [--csv out.csv]` rechnet Stunden je Projekt und Tag bzw. Woche (das Skript braucht nur Python 3). Die Pause wird nicht summiert, Abschnitte über Mitternacht werden geteilt. Summiert wird nach dem Namen aus dem Log: Nach einer Umbenennung stehen alter und neuer Name in getrennten Spalten.

### Laden und Akku
- **Laden:** USB-C-Kabel in das Loch an Seite 3 stecken (das Loch misst 13,5 × 7 mm; schlanke Stecker passen, sehr dicke Umspritzungen eventuell nicht).
- **Akkustand:** Die Akkuspannung steht rechts oben in der Tagesübersicht und als `vbat_mv` im Log. Die Laufzeit pro Ladung ist nicht gemessen.

### Wenn etwas nicht klappt
| Beobachtung | Was tun |
|---|---|
| Die Seite wird nicht erkannt, springt oder kommt zu spät | Die Münze muss ungefähr auf dem Rand stehen (höchstens 35° gekippt) und rund 0,4 s ruhig liegen. Hilft das nicht: neu einlernen (Taste A 6 s). |
| Schütteln im Liegen zeigt nichts | Die Münze muss flach auf dem Rücken liegen, und es darf gerade nichts angezeigt werden. Kräftiger und etwas länger schütteln (rund 0,5 s nach dem Start). Alternativ Taste A kurz drücken. |
| Display „Zeit unbekannt“ | Der Stick konnte die Uhrzeit nicht holen (WLAN oder Internetzeit). Er versucht es bei einem Projektwechsel und beim Datenabruf erneut. |
| Datenabruf zeigt „keine Verbindung“ (drei tiefe Töne) | Das WLAN ist nicht erreichbar. Der Stick braucht das eingestellte 2,4-GHz-Netz. |
| „Teach-in abgebrochen (Zeit)“ oder „Reihenfolge falsch?“ | Das Einlernen noch einmal starten (Taste A 6 s), die Seiten der Reihe nach, jede Lage 1 s ruhig. |

### Bekannte Einschränkungen
- **Anzeige-Drehung:** Beim Aufstellen dreht sich der Text. Ob er überall aufrecht steht, ist noch nicht abgestimmt, und auf den schrägen Seiten (1, 3, 5, 7) steht er um 45° schief.
- **Schnelles Rollen:** Ob Zwischenseiten beim sehr schnellen Rollen zuverlässig ausgeblendet werden, ist nicht geprüft.
- **Noch nicht am Gerät beobachtet:** Einlernen per Taste A (6 s).
- **Nicht gemessen:** Akkulaufzeit und Genauigkeit der Uhr im Schlaf. Die Uhr wird bei einem Projektwechsel per Internet nachgestellt, wenn der letzte Abgleich mehr als 6 h her ist.

Prüfergebnisse im Einzelnen: Teil 2, „Stand der Prüfung“.

## Teil 2: Das Projekt
Für alle, die den Stick bauen, flashen oder weiterentwickeln. Hardware: M5Stack StickS3 (ESP32-S3, BMI270, M5PM1) in einer 3D-gedruckten Münze; Software: PlatformIO (Arduino, M5Unified, M5PM1). Entwicklung (Hardware-Fakten mit Quellen, Befehle, Konventionen, nächste Schritte): siehe `CLAUDE.md`.

### Varianten und Verzeichnisse
| Variante | Gehäuse | Firmware-Env |
|---|---|---|
| **8-Eck-Münze, 22 mm, mit USB-C-Loch** | `zeitprisma_muenze8_22mm_usb.stl` | `muenze8` (Standard) |
| 12-Eck-Münze, 18,6 mm | `muenze_gehaeuse.py --seiten 12 --dicke 18.58 --usb nein` | `muenze12` |
| Zehnkant-Prisma (Körper + Deckel) | `prisma_gehaeuse.py` (erzeugt `zeitprisma10_koerper.stl`, `zeitprisma10_deckel.stl`) | `prisma10` |

```
gehaeuse/   muenze_gehaeuse.py, prisma_gehaeuse.py (parametrisch), fertige STL, Vorschau (vorschau_muenze.py rendert sie)
firmware/   PlatformIO-Projekt (Arduino, M5Unified, M5PM1), Envs muenze8 | muenze12 | prisma10
tools/      auswertung.py (CSV -> Stunden je Projekt und Tag/Woche), seriallog.py (serielle Ausgabe mitschreiben)
Makefile    make test | build | flash | log | ref | gehaeuse | vorschau
```

### Gehäuse (8-Eck-Münze)
- **Druck:** ein Teil, Rückseite aufs Bett, ohne Stützen. Einzige Brücke ist das USB-Loch (13,5 mm). Ca. 15 g PLA (gerechnet, 35 % Füllgrad angenommen; je nach Slicer-Einstellung vermutlich mehr).
- **Einbau:**
  1. M8-Unterlegscheibe DIN 125 (16 × 1,6 mm, Stahl) in die Mulde im Taschenboden legen, eventuell mit einem Tropfen Kleber. Sie hält den Stick über seinen Magnetrücken.
  2. Stick mit dem USB-Ende zur Seite „3“ einlegen (die Seite mit dem USB-Loch).
  3. Herausnehmen: Die Rückseite ist geschlossen (ohne Ausdrückloch). Der Stick sitzt nur magnetisch auf der Scheibe und muss von vorne herausgehoben werden, z. B. mit einem Plektrum oder Fingernagel am Rand der Tasche (rundum 0,3 mm Spiel, die Vorderseite des Sticks liegt 1,5 mm unter der Münzfläche). Ob das praktikabel ist, hat der Nutzer nicht gesondert gemeldet (siehe Stand der Prüfung); `muenze_gehaeuse.py --ausdrueckloch` erzeugt ein Ø10-Ausdrückloch in der Rückseite.
- **Laden:** USB-C-Loch in Seite 3, 13,5 × 7 mm. Es passen Stecker bis ca. 12,4 × 6,7 mm.
- **Seitentaster** (Reset/Einschalten, Download-Modus, siehe Abschnitt Flashen) sind bei eingebautem Stick **nicht erreichbar**; die Münze hat dafür keine Löcher. Das Zehnkant-Prisma hat Ø5,5-Zugangslöcher.
- **Stabilität (gerechnet, nicht gemessen):** Aufgestellt kippt die Münze ab ca. 21° zur Nachbarseite und ab ca. 20° flach um. Zum Vergleich: die 12-Eck-Münze mit 18,6 mm kippte ab ca. 13°.
- **Ziffern:** 1–8 auf den Randflächen, ohne Punkt, graviert und lesbar von vorne-oben, wenn das Display zu einem zeigt. Die „1“ liegt bei −Y im Modell, die Zählung läuft von dort weiter, die USB-Seite ist die „3“. Gravurtiefe 1,0 mm: mehr lässt die Stirnwand an den Seiten 3 und 7 nicht zu (dort bleiben 2,6 mm Wand, nach der Gravur genau 1,6 mm = `WALL_MIN`); das Skript begrenzt die Tiefe selbst auf die Restwand (`digit_depth()`, bei der 12-Eck-Variante bleibt es bei 0,6 mm, dort bleiben 1,0 mm).
- **Innentext:** Innen an den beiden langen Taschenwänden ist Text eingraviert (0,6 mm tief, Wand dahinter über 14 mm): „Octochron“ an der +Y-Wand, an der −Y-Wand in zwei Zeilen die Namen der Urheber (`TEXT_RIGHT`). Er ist von innen lesbar und erst ohne Stick zu sehen. Er steht in `muenze_gehaeuse.py` (`TEXT_LEFT`, `TEXT_RIGHT`, Schrifthöhe und Tiefe daneben). Bei 3 bis 5 mm Schrifthöhe ist er im FDM-Druck vermutlich nur grob lesbar (Strichstärke ca. 0,7 bis 1,1 mm).
- **Anpassen:** `python3 muenze_gehaeuse.py --seiten N --dicke D --usb auto|ja|nein [--ausdrueckloch] --check ../ref/StickS3.stl` (Abhängigkeiten: `pip install manifold3d numpy matplotlib`, geprüft mit manifold3d 3.5.4; das Referenzmodell lädt `make ref`). Das Vorschaubild (`vorschau_muenze8.png`: Rücken unten, Rückseite, aufgestellt mit „1“ oben, USB-Seite oben, beide Innenwände) entsteht mit `make vorschau` aus dem Modell, mit einem kleinen eigenen Z-Buffer-Renderer ohne Zusatzprogramme.
- **Am PC geprüft:** `--check` besteht für die Münze 8 (13 Punkte, darunter „Rückseite geschlossen“, „Ziffern: Restwand“ und beide Innentexte) und die Münze 12 (11). Die STL ist wasserdicht (jede Kante in genau zwei Dreiecken), 33,0 cm³, 53,8 × 53,8 × 22,0 mm, Kippgrenze 20,8° zur Nachbarseite und 20,2° flach. Ansicht jeder der 8 Randseiten von vorn: Nummer 1 bis 8 in der Reihenfolge −Y, −X−Y, −X (USB), −X+Y, +Y, +X+Y, +X, +X−Y.
- **Gedruckt:** Das Gehäuse funktioniert laut Nutzer (Einzelheiten: Stand der Prüfung).

Seitennummern (Blick auf das Display, USB-C-Loch links; aus den Modellkoordinaten abgeleitet):

```
              5
         ___________
        /           \
    4  /             \  6
      |               |
  3   |    Display    |   7
 (USB)|               |
      |               |
    2  \             /  8
        \___________/
              1
```

### Einstellungen (WLAN und Projektnamen)
WLAN-Zugangsdaten und Projektnamen stehen fest in der Firmware und ändern sich nur durch Neu-Flashen.
1. `firmware/include/config_local.h.example` nach `config_local.h` kopieren und dort WLAN-Zugangsdaten und Projektnamen eintragen (`PROJECT_NAMES_LOCAL`: „Pause“ plus die Projekte, ASCII). Die Datei steht in `.gitignore` und kommt nie nach GitHub, so landen auch Kundennamen nicht im öffentlichen Repo. Ohne die Namenszeile gelten „Projekt 1“ … aus `config.h`.
2. Flashen: siehe den nächsten Abschnitt.
3. Alle Zeiten, Schwellen und Töne stehen in `firmware/include/config.h`, die Abläufe im Einzelnen unter „Funktionsweise im Detail“.

Das eingelernte Seitenmodell bleibt beim Flashen derselben Bauform erhalten; bei einer anderen Bauform (Env) oder nach dem Löschen des NVS folgt ein neues Teach-in.

### Flashen
- **Normalfall:** Stick wach und am USB (Linux: Port `/dev/ttyACM0`, Benutzer in der Gruppe `dialout`), dann `make flash ENV=muenze8` (= `cd firmware && pio run -e muenze8 -t upload`; `pio` muss im PATH sein). esptool setzt den Stick selbst in den Bootloader, startet die App danach aber nicht (siehe „Nach dem Flashen die App starten“).
- **Wenn kein Port erscheint oder der Stick im Sekundentakt neu startet:** Download-Modus erzwingen. Das Neustart-Verhalten zeigt sich im Kernel-Log als ständig neue USB-Gerätenummern; mögliche Ursache ist die Wecken-Schleife (siehe Fehlersuche). Laut M5-Doku: USB-Kabel anschließen und die **seitliche Reset-Taste gedrückt halten, bis die interne grüne LED blinkt** (eine Haltedauer nennt die Doku nicht). Danach bleibt `303a:1001` (`lsusb`) dauerhaft stehen, ohne neue Gerätenummern. Das hat am Gerät funktioniert.
- **Prüfen, ohne den Download-Modus zu verlassen:** `esptool.py --chip esp32s3 --port /dev/ttyACM0 --after no_reset flash_id`. Erwartet: ESP32-S3-PICO-1, 8 MB Flash, 8 MB PSRAM, am Ende „Staying in bootloader.“
- **Bootloader oder laufende App?** `esptool.py ... --before no_reset --after no_reset chip_id`: Antwortet der Chip (MAC wird gezeigt), steht er im Bootloader. Die laufende App antwortet nicht.
- **Nach dem Flashen die App starten:** Der „Hard resetting via RTS pin“ von esptool lässt den Stick hier im Download-Modus stehen. Gemessen: Nach einem RTS-Reset meldet das ROM `rst:0x15 (USB_UART_CHIP_RESET),boot:0x21 (DOWNLOAD(USB/UART0))` und „waiting for download“, die App startet nicht. **Ein Watchdog-Reset startet die App:** `esptool.py --chip esp32s3 --port /dev/ttyACM0 --before default_reset --after watchdog_reset chip_id` (esptool liegt unter `~/.platformio/packages/tool-esptoolpy/`; der USB-Port meldet sich dabei neu an, die Port-Nummer kann sich ändern). Ein Klick auf die Seitentaste wurde nicht gezielt getestet (M5-Doku: Einfachklick = Einschalten/Reset, Doppelklick = Ausschalten).
- **ModemManager (Linux):** Er kann den Port des Sticks offen halten (`fuser -v /dev/ttyACM*`); dann wandert die Port-Nummer, z. B. von `ttyACM0` auf `ttyACM1`. ModemManager ist ein bekannter Störer bei ESP32-Geräten mit USB (öffnet Ports, setzt DTR/RTS, schickt AT-Befehle). PlatformIO meldet beim Upload außerdem, dass `/etc/udev/rules.d/99-platformio-udev.rules` veraltet ist. Vermutlich hilft es, die Regeln zu erneuern: `curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules`, danach `sudo udevadm control --reload-rules && sudo udevadm trigger` und den Stick neu anstecken (Befehle laut PlatformIO-Doku). Ob das den hängenden Download-Modus nach dem Flashen und das Wandern der Port-Nummer behebt, ist **ungeprüft**.
- **Mitlesen:** `make log` (= `tools/seriallog.py -s 30`, braucht pyserial, z. B. mit dem Python von PlatformIO: `~/.platformio/penv/bin/python tools/seriallog.py -s 30`). Die Startzeile erscheint rund 1 s nach dem Reset, meist bevor der Rechner den Port geöffnet hat. Deshalb den Logger **vor** dem Reset starten.
- **Deep Sleep:** Im Deep Sleep verschwindet der USB-Port (laut `seriallog.py`), zum Flashen den Stick bewegen oder Taste A drücken. Bei eingebautem Stick gibt es keinen Zugang zur Seitentaste.
- **Schlafenden Stick flashen:** Der Port ist nur 1 bis 3 s nach einer Bewegung da. Eine Schleife prüft alle 50 ms, ob `/dev/ttyACM*` erscheint, und startet dann sofort `esptool.py --chip esp32s3 --port <Port> --before default_reset --after no_reset --connect-attempts 1 flash_id` (mit `timeout 15`). Damit steht der Chip im ROM-Bootloader und bleibt dort stehen. Danach `make flash ENV=muenze8` und der Watchdog-Reset aus dem Punkt „Nach dem Flashen die App starten“. Der Stick muss während der Schleife bewegt werden (Rollen genügt).

### Stand der Prüfung
Stand 30.10.2026, Einschränkungen zuerst.

| Bereich | Stand |
|---|---|
| Teach-in im Gehäuse (nicht gesondert gemeldet), Taste A 6 s, Darstellung eigener Projektnamen am Display, Anzeige-Drehung, Stromverbrauch und Laufzeit | **nicht geprüft** |
| Gehäuse (gedruckt) | funktioniert laut Nutzer (ohne Log, ohne Einzelheiten) |
| Töne, Projektanzeige mit Tageszeit, Schütteln im Liegen, Taste A kurz | vom Nutzer am Gerät bestätigt (ohne Log) |
| Teach-in (ohne Gehäuse) | lief am Gerät durch (9 Seiten, Abweichung max 6,0°) |
| WLAN-Export (Taste A 2 s), Tagesdateien, Gesamtdatei, Tageswechsel um Mitternacht (Übertragszeile, neue Tagesdatei) | am Gerät abgerufen und geprüft |
| Start, Wecken per Bewegung, Seitenerkennung beim Rollen von Hand (ohne Gehäuse), eigene Projektnamen aus `config_local.h` im Log | am Gerät im Log gemessen |

**Nicht am Gerät geprüft:**
- Teach-in im Gehäuse (der Nutzer meldet das Gehäuse insgesamt als funktionierend, das Teach-in darin nicht gesondert) und Taste A 6 s halten (Teach-in per Taste).
- Darstellung der eigenen Projektnamen auf dem Display (im Log stehen sie; die Übersicht kürzt Namen auf 7 Zeichen).
- Stromverbrauch und Laufzeit, Anzeige-Drehung des aufgestellten Textes (Kalibrierung `DISP_ROT_OFFSET/DIR`), schnelles Rollen mit der kurzen Entprellung.
- Zeiten für Schütteln und Wecken per Taste (nicht gemessen).

**Am PC geprüft:** Kernlogik in Host-Tests (`make test`: Münze 8 = 262, Münze 12 = 352, Prisma 10 = 281 Prüfungen, alle bestanden), Gehäuse-Passung gegen das M5-Strukturmodell (rechnerisch) und dass alle drei Firmware-Varianten mit der ESP32-S3-Toolchain kompilieren (`make build`, 0 Warnungen). „Kompiliert“ heißt nicht „läuft“.

**Am Gerät im Log gemessen** (Stick ohne Gehäuse, von Hand gerollt):
- **Start:** Die Firmware (`muenze8`) startet: Startzeile im Log, M5PM1 (Akkuspannung gelesen) und BMI270 ohne Fehlermeldung initialisiert, WLAN offenbar verbunden.
- **Seitenerkennung:** 200-s-Mitschnitt mit 26 Seitenwechseln (alle 8 Randseiten und Pause), **kein einziger „unbekannt“**, kein Wechsel im selben Wachabschnitt unter 1,4 s Abstand (das ist das Rolltempo), Startzeile 0,57 s nach dem Chip-Start, erste Seite 1,0 bis 1,3 s nach dem Chip-Start.
- **Wecken per Bewegung:** Bewegung weckt den Stick (4 und 8 Mal in zwei 200-s-Mitschnitten), unberührt schläft er ohne Aufwachen (84 s, 35 s und 55 s am Stück gemessen), es gibt keine Wecken-Schleife (Hintergrund: Diagnose der Weckkette).
- **Projektnamen:** Die eigenen Namen aus `config_local.h` stehen im Log (`Seite n -> Name`, alle 9 Seiten der Münze 8 gesehen).
- **Teach-in** (ohne Gehäuse): lief durch (9 Seiten, Abweichung max 6,0°), die erste Tagesdatei wurde angelegt.
- **WLAN-Export:** Taste A 2 s halten startet ihn, der Stick ist danach per WLAN erreichbar. Abgerufen und geprüft: Startseite, Tagesdatei und Alt-Datei einzeln, Gesamtdatei auch per HTTP/1.1 (siehe Log-Datei).
- **Tageswechsel:** Die neue Tagesdatei beginnt mit der Übertragszeile `00:00:00` (Projekt des Vortags, `vbat_mv` 0, `epoch` = lokale Mitternacht) und wurde mit dem ersten Wechsel des Tages angelegt (Kopf, Übertragszeile, Ereignisse). In beiden Tagesdateien sind die Zeiten aufsteigend und passen zu `epoch`, `zeit_ok` ist überall 1, `seite` und `projekt` widersprechen sich nirgends. Die Tagessummen auf der Startseite begannen bei 0.

**Vom Nutzer am Gerät bestätigt** (ohne Log):
- Die Töne sind gut (8 verschiedene weiche Einzeltöne), beim Wecken aus dem Liegen in ein Projekt kommt der Ton, die Projektanzeige (5 s, mit der aufsummierten Tageszeit des Projekts) passt, Schütteln im Liegen zeigt die Tagesübersicht, und Taste A zeigt sie nach einem kurzen Klick aus dem Deep Sleep.
- Gehäuse: gedruckt und funktioniert. Welche Modellversion gedruckt wurde und welche der Annahmen im Abschnitt Gehäuse (Entnahme ohne Ausdrückloch, Magnet-Halt, Kippverhalten, USB-Stecker, Lesbarkeit von Ziffern und Innentext) damit geprüft sind, ist nicht angegeben; sie sind deshalb hier nicht einzeln als geprüft vermerkt.

### Funktionsweise im Detail (aus dem Code, Stand 29.10.2026; Konstanten in `firmware/include/config.h`)
Alles hier ist aus dem Code gelesen. Was davon am Gerät bestätigt ist, steht im Abschnitt „Stand der Prüfung“ (nicht bestätigt: Teach-in im Gehäuse, Taste A 6 s, Stromverbrauch).

#### Ablauf im Überblick
1. Im Deep Sleep überwacht der Beschleunigungssensor BMI270 die Bewegung selbst (any-motion, Schwelle ca. 80 mg über 100 ms).
2. Bewegung löst einen Interrupt aus. Der Power-Chip M5PM1 zieht die Weckleitung G13 auf Low und weckt den ESP32-S3. Taste A weckt ihn direkt über G11.
3. Der Stick startet (Warmstart, rund 0,6 s bis zur Startzeile im Log, ROM und Bootloader nicht mitgerechnet) und liest die Lage 50-mal je Sekunde.
4. Liegt der Stick 0,12 s ruhig und eine neue Seite danach 0,25 s ununterbrochen, wird der Wechsel übernommen: Zeit gutschreiben, Zeile loggen, Ton, Anzeige.
5. Ist nichts mehr zu tun, schläft er wieder ein. Seite, Beginn der Strecke und Tagessummen bleiben im RTC-RAM und überstehen den Schlaf, nicht aber Stromverlust und Flashen. Das Seitenmodell aus dem Teach-in (NVS) und die Logs (LittleFS) liegen im Flash.

#### Gespeicherter Zustand
- **RTC-RAM** (überlebt Deep Sleep, nicht Stromverlust, Flashen oder Bauform-Wechsel): aktive Seite, Beginn des Abschnitts, Tagessummen je Projekt, Zeitpunkt des letzten NTP-Syncs, „Zeit gültig“, Zähler unruhiger Sitzungen. Prüfwert `0x5A500300 | Seitenzahl`. Stimmt er nicht, ist es ein **Kaltstart**. Dazu der Merker `imuFastOk` in `hal.cpp` (Warmstart ohne M5-IMU-Init erlaubt, siehe „Start“).
- **NVS** (Namespace `prisma`): Teach-in-Seitenmodell, Version `0x300 | Seitenzahl`. Passt die Version nicht, gilt das Modell als ungültig und es folgt ein Teach-in.
- **LittleFS:** die Logs (`/log_JJJJ-MM-TT.csv` und `/log_ohne_zeit.csv`, dazu die Alt-Datei `/log.csv`, siehe Log-Datei). Ist das Dateisystem leer oder defekt, wird es formatiert.

#### Start
1. Display quer (Rotation 1), M5PM1 und BMI270 initialisieren. Bei Fehler zeigt das Display „Hardwarefehler / PM1/IMU pruefen“ (beim Kaltstart gleich überschrieben), seriell steht „M5PM1 nicht erreichbar“ bzw. „BMI270 nicht gefunden“.
   - **Schneller Warmstart (nur nach dem Wecken aus dem Deep Sleep):** M5Unified initialisiert das BMI270 bei jedem `M5.begin()` neu (Reset, 8-KB-Konfiguration hochladen, `INT_MAP_DATA` = 0xFF schreiben, aus `BMI270_Class.inl` gelesen). Nach dem Deep Sleep ist das BMI270 aber noch konfiguriert. Ist der RTC-Merker `imuFastOk` gesetzt, läuft `M5.begin()` deshalb mit `internal_imu = false`, und `hal` liest das BMI270 selbst (Rohwerte 0x0C bis 0x11, Achsen `{Y, −X, Z}` wie M5Unified, Messbereich aus `ACC_RANGE`). `hal::begin()` prüft dabei `INTERNAL_STATUS` (Init ok = 1) und holt sonst die M5-Initialisierung nach (Warnung „BMI270 nicht mehr initialisiert, M5-Init nachgeholt“). Im Warmstart ist `M5.Imu` nicht gestartet: Code außerhalb von `hal.cpp` darf es nicht direkt aufrufen, nur `hal::readAccel()`.
   - **Abgleich, der den Merker setzt** (`imuSelfCheck()`, bei jedem Start **mit** M5-IMU-Init, also Kaltstart und Warmstart ohne Merker): Die eigene Lesung und `M5.Imu.getAccel` dürfen höchstens 0,08 g auseinanderliegen, und beide müssen eine Schwerkraft von 0,8 bis 1,2 g zeigen (sonst könnten zwei Nullvektoren „übereinstimmen“). Das sichert ab, dass das angelernte Seitenmodell zum selben Koordinatensystem gehört. Liegt der Stick beim Start nicht ruhig, fällt der Abgleich durch, und der nächste Start prüft erneut. Ergebnis seriell: `IMU-Abgleich: M5 (x y z) eigen (x y z) max. Abweichung .. g, |g| ../.. -> schneller Warmstart erlaubt|NICHT erlaubt`.
   - **Gemessen:** Startzeile 0,57 s nach dem Chip-Start mit eigenem Lesen (`begin` = 434 ms), 0,82 s mit M5-IMU-Init (`begin` = 664 ms), beim Kaltstart 1,0 s. Der Schnellstart spart gegenüber dem M5-Init also ca. 230 ms. Der Abgleich stimmte bei zwei Lagen auf 0,000 g überein; bei Bewegung (|g| 1,62) lehnt er ab. Der Rest der Bootzeit ist nicht weiter aufgeschlüsselt (siehe die Felder der Startzeile unter „Serielle Ausgabe“). Die Zeit von Reset bis Chip-Start (ROM, Bootloader) steckt in diesen Zahlen nicht drin und ist ungemessen.
2. Seitenmodell laden, sonst Notlösung `defaultModel()` (ungültig).
3. Serielle Startzeile (siehe „Serielle Ausgabe“).
4. **Kaltstart:** Display „Zeitprisma / hole Uhrzeit ...“, WLAN (max. 8 s) und NTP (max. 6 s), dann bei ungültigem Modell Teach-in, dann Status für 5 s.
5. **Wecken per Taste A:** Status für 8 s. **Wecken per Bewegung:** Display bleibt aus, bis es etwas zu zeigen gibt. Der Lautsprecher wird gleich vorgestartet (Anlaufzeit, siehe Anzeige und Töne), und liegend (Seite 0) bleibt der Stick `SHAKE_GRACE_MS` = 2 s wach, damit ein Schütteln erkannt wird.

#### Seitenerkennung (alle 20 ms)
- **Ruhig** heißt: 6 Samples (0,12 s, `STILL_WINDOW`) weichen alle weniger als 3° vom Mittel ab, und der Betrag liegt zwischen 0,8 und 1,2 g. Nur dann wird die Seite bestimmt, sonst bleibt der bisherige Kandidat.
- **Ringerkennung ohne Totzone** (`classifyRing()` in `prisma_core`): Liegt der Beschleunigungsvektor ca. in der Ringebene („Stick in der Drehachse“, Rollachse waagrecht), wird **immer** die nächste Randseite geliefert, auch zwischen zwei Seiten. Die Ringebene steht senkrecht zur Drehachse (Münze: Display-Normale, Prisma: Längsachse). Verglichen wird der Winkel in der Ringebene, ein Kippen aus der Ebene verfälscht ihn nicht.
- **Hysterese**, jeweils bezogen auf die bisherige Seite (der aktuelle Kandidat):
  - *Nachbarseiten:* Gewechselt wird erst, wenn eine andere Seite um mehr als `RING_HYST_DEG` näher liegt (20 % des Seitenabstands, bei 8 Seiten 9°). Bei 8 Seiten schaltet es von Seite i zu i+1 bei 27° Abstand zur Mitte von i, zurück erst bei 18°. Dazwischen bleibt die bisherige Seite.
  - *Ringebene:* Eintritt bei höchstens 35° Kippung aus der Ebene (`RING_UPRIGHT_DEG`), Verbleib bis 45° (`RING_UPRIGHT_EXIT_DEG`).
  - *Münze auf dem Rücken (Seite 0):* Eintritt bei höchstens 30° von „Display oben“ (`FLAT_DEG`), Verbleib bis 40° (`FLAT_EXIT_DEG`).
- **Außerhalb** davon ist die Lage „unbekannt“ (−1) und zählt als Pause: Münze mit Display unten oder stärker schräg, Prisma aufrecht stehend. Beim Prisma gehört Seite 0 zum Ring.
- Eine neue Seite wird erst übernommen, wenn sie **`SETTLE_MS` = 0,25 s** ununterbrochen ruhig lag, nach dem 0,12 s Ruhefenster also rund 0,4 s nach dem Stillstand (Abtasttakt 20 ms). Bewegung setzt den Timer zurück. Nach dem Wecken aus dem Deep Sleep kommt der Start dazu (die Startzeile erscheint 0,57 s nach dem Chip-Start, siehe „Start“). Gemessen: Der erste Wechsel stand 1,0 bis 1,3 s nach dem Chip-Start im Log, davon rund 0,4 s Erkennung, wenn der Stick sofort ruhig lag.
- Kürzere Zeiten erkennen schneller, aber Zwischenseiten beim schnellen Rollen werden eher mitgezählt: Wer eine Seite länger als `STILL_WINDOW` + `SETTLE_MS` ruhig lässt, bekommt sie als Wechsel. Wurden beim Rollen falsche Zwischenseiten gezählt, `SETTLE_MS` erhöhen. Im Mitschnitt war das nicht zu sehen (die Reihenfolge der Seiten war lückenlos), schnelleres Rollen ist aber nicht geprüft.
- **Übernehmen:** Zeit des vorigen Abschnitts dem alten Projekt gutschreiben (nur bei gültiger Zeit, an Mitternacht geteilt, im Gerät bleibt nur der heutige Tag), CSV-Zeile anhängen, seriell „Seite n -> Name“, Töne, Anzeige (Seite 0: Status 8 s; Randseite: Projektfarbe, Name und Tageszeit des Projekts 5 s, aufrecht gedreht), NTP-Sync, wenn der letzte älter als 6 h ist.
- Projektnummer: Seite 0 und „unbekannt“ = Projekt 0 (Pause), Seite k = Projekt k.

#### Teach-in
- **Start:** automatisch bei Kaltstart ohne gültiges Modell, sonst Taste A 6 s halten und loslassen.
- **Ablauf:** Display „Teach-in / Display nach oben legen / und ruhig liegen lassen“ und langer Ton. Erst Seite 0 (Display oben, flach), dann nacheinander Seite 1 bis 8 nach oben bringen (Münze auf den Rand stellen). Jede Lage muss **1 s ruhig** liegen. Bestätigung: Ton und „Seite k erfasst / weiter: k+1 nach oben“, am Ende „fertig“.
- **Annahmeregeln** (sonst wird die Lage still ignoriert): mindestens 0,65 × Schrittwinkel (bei 8 Seiten 29°) Abstand zu allen bisherigen. Ab der zweiten Randseite liegt jede etwa einen Schrittwinkel (±40 %, bei 8 Seiten 27° bis 63°) neben der zuletzt erfassten. Randseiten liegen höchstens 30° aus der Randebene, also 60° bis 120° zu Seite 0.
- **Zeitlimit:** 180 s je Seite. Danach „Teach-in / abgebrochen (Zeit)“, drei tiefe Töne, das alte Modell bleibt.
- **Abschlussprüfung:** Nachbarseiten im Ring 45° ± 8°, Randseiten 90° ± 8° zu Seite 0. Sonst „Reihenfolge falsch?“ mit „Abweichung max x Grad“, drei tiefe Töne, altes Modell bleibt. Erfolg: „Teach-in OK / gespeichert“, langer Ton, Modell im NVS.
- Von Hand ohne Gehäuse lief das Teach-in mit 6,0° Abweichung durch (Grenze 8°, ein einziger Versuch). Im gedruckten Gehäuse sollten die Lagen genauer stimmen (ungeprüft).
- **Seriell:** `Teach-in: Seite k erfasst` je Seite, am Ende `Teach-in: alle 9 Seiten erfasst, Abweichung max x.x Grad -> Modell gespeichert|verworfen (Reihenfolge falsch?)`, bei Zeitüberschreitung `Teach-in: abgebrochen (Zeit), n von 9 Seiten erfasst`. Der Wert „Abweichung max“ ist die Prüfgröße der Abschlussprüfung (Grenze 8°).
- **Neu einlernen erzwingen:** Ohne Rechner Taste A 6 s halten und loslassen. Vom Rechner aus das Modell im NVS löschen, dann startet der nächste Kaltstart ohne Modell das Teach-in von selbst (nach „hole Uhrzeit“, WLAN und NTP brauchen bis ca. 14 s): Stick im Bootloader (siehe Flashen, „Schlafenden Stick flashen“), dann `esptool.py --chip esp32s3 --port <Port> --before default_reset --after watchdog_reset erase_region 0x9000 0x5000` (Partition `nvs` laut `default_8MB.csv`: Offset 0x9000, Größe 0x5000). Das löscht auch die WLAN-Kalibrierdaten (werden beim nächsten WLAN-Start neu angelegt). Vorher sichern: `esptool.py ... --after no_reset read_flash 0x9000 0x5000 nvs_backup.bin` (die Datei kann je nach WLAN-Speicherung Zugangsdaten enthalten: nicht ins Repo legen). Misslingt das neue Teach-in („Reihenfolge falsch?“), bleibt der Stick ohne Modell, bis ein Teach-in gelingt (Taste A 6 s).

#### Schlafen und Wecken
- **Wann:** Sobald nichts mehr angezeigt wird, Taste A nicht gedrückt ist und die Seite übernommen und stabil ist. Spätestens 45 s nach Sitzungsbeginn (z. B. in der Tasche), dann zählt die Sitzung als „unruhig“. Nach 3 unruhigen Sitzungen in Folge: 5 min Timer-Schlaf.
- **Deep Sleep, Ablauf:** Display aus, Backlight/Mikrofon/Lautsprecher (L3B) über M5PM1 PYG2 aus. Das BMI270 wird für any-motion konfiguriert (Schwelle `ANYMOT_THRESHOLD` 0xAA, ca. 80 mg, Dauer 5 Samples = 100 ms) und darf **nur INT1** treiben: `INT_MAP_DATA`, `INT2_MAP_FEAT` und `INT2_IO_CTRL` werden auf 0 geschrieben (siehe Diagnose der Weckkette). Dann wird der M5PM1 scharf gestellt (nur GPIO4 entmaskiert, PYG1 als IRQ-Ausgang). Danach bleibt der Stick wach, löscht den IRQ bei jeder Auslösung und wartet, bis G13 `IRQ_QUIET_MS` = 300 ms ruhig High war (höchstens `IRQ_SETTLE_MAX_MS` = 2,5 s). Erst dann beginnt der Deep Sleep.
- **Wecken:** Bewegung (BMI270 INT1 → M5PM1 PYG4 → IRQ PYG1 → G13, EXT0 low) oder Taste A (G11, EXT1 low). Kommt G13 nicht zur Ruhe (echte Dauerbewegung, z. B. in der Tasche) oder wird die Kette nicht scharf, gilt ein Timer-Rückfall von 60 s und seriell steht „Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s“.
- **Wecken per Taste A:** `wokeByButton()` ist wahr, wenn der Weckgrund EXT1 (Deep Sleep) oder GPIO (Light Sleep) ist oder die Taste beim Start noch gedrückt ist; dann zeigt `setup()` die Tagesübersicht (`STATUS_SHOW_MS`). Im Log steht ein Tasten-Wecken als `Weckgrund=3`. Bewegung und Taste zugleich melden nur einen Grund (ESP-IDF bevorzugt EXT0, also Bewegung); dann kommt die Anzeige nur, wenn die Taste noch gedrückt ist.
- **Schütteln im Liegen (Tagesübersicht):** Bewegung weckt den Stick immer, aber ohne Seitenwechsel bliebe das Display aus. Deshalb wertet `loop()` zusätzlich den `ShakeDetector` aus (`SHAKE_JERK_G` 0,25 g, `SHAKE_PEAKS` 4, `SHAKE_WINDOW_MS` 1000): vier Spitzen der Änderung zwischen zwei Samples (20 ms) über 0,25 g innerhalb 1 s gelten als Schütteln. Am PC getestet (Host-Tests): 1,0 g bei 3 Hz wird nach 540 ms erkannt, 1,5 g bei 2 Hz nach 780 ms, 0,3 g bei 3 Hz nicht; einzelne Stöße, zwei bis drei Klopfer, Rauschen, Rollen und schnelles Umdrehen lösen nicht aus. Dann zeigt der Stick die Tagesübersicht (`STATUS_SHOW_MS`), aber nur, wenn Seite 0 (liegend, Display oben) übernommen ist, die letzte Sitzung nicht „unruhig“ war und gerade nichts angezeigt wird. Das Schütteln muss nach dem Boot (ab ca. 0,6 s) noch rund 0,5 s weitergehen. Seriell: `Schuetteln erkannt: Uebersicht` bzw. `ignoriert (...)`. **Vom Nutzer am Gerät bestätigt:** Die Übersicht erscheint beim Schütteln im Liegen. Nicht gemessen: wie lange man schütteln muss, und ob sanftes Schütteln das Wecken (any-motion, ca. 80 mg Änderung je Sample über 100 ms) auslöst; die Schwellen im Alltag weiter beobachten. Beim Rollen von Hand meldet der Detektor „Schuetteln erkannt: ignoriert“ (im Mitschnitt 6 Mal in 25 s), er reagiert also auch auf schnelles Umdrehen. Das ist folgenlos, weil die Übersicht nur bei übernommener Seite 0 erscheint. Blitzt sie beim Hochheben einer flach liegenden Münze auf, `SHAKE_PEAKS` erhöhen oder `SHAKE_JERK_G` anheben.
- `POWER_DEEPSLEEP 0` schaltet auf Light Sleep um, alle 700 ms wird die Lage geprüft.

#### Diagnose der Weckkette (`WAKE_DIAG` in `config.h`)
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

**Hintergrund: Wecken-Schleife durch `INT_MAP_DATA`.** Nach `M5.begin()` steht `INT_MAP_DATA` (0x58) des BMI270 auf `0xFF`. Das legt Daten-bereit-, FIFO- und Fehler-Interrupts zusätzlich auf INT1 und INT2. INT1 pulst dann im 50-Hz-Takt des Sensors, der M5PM1 merkt sich die Flanken (`g=10`, GPIO4) und zieht G13 auf Low, sodass der Stick direkt nach dem Einschlafen wieder geweckt wird (Aufwachen ca. 0,6 s nach dem Einschlafen, dreimal, dann Timer-Rückfall von 60 s). Im Trace erscheinen die Impulse nur auf einem Raster von ca. 20,1 ms (Abstände von 4, 7, 8, 10, 11 oder 15 Samples), weil das 1,3-ms-Leseraster die kurzen Impulse nur zufällig trifft. Auch ein Löschen des IRQ nach 30 ms hilft nicht. Die any-motion-Konfiguration selbst ist davon unabhängig richtig. **Gegenmaßnahme:** beim Armieren `INT_MAP_DATA`, `INT2_MAP_FEAT` und `INT2_IO_CTRL` auf 0 schreiben und vor dem Einschlafen warten, bis G13 300 ms ruhig High ist. Wirkung (am Gerät gemessen): keine INT1-Impulse mehr im Trace, `ruhig nach 300 ms, 0 Ausloesung(en)`, G13 bleibt High, der Stick bleibt im Deep Sleep. Der Hub liegt bei ruhig liegendem Stick unter 40 mg, deutlich unter der any-motion-Schwelle von ca. 80 mg.

#### Anzeige und Töne
- **Status** (Seite 0 oder Taste A kurz): „Heute dd.mm. HH:MM“ (oder „Zeit unbekannt“), rechts oben die Akkuspannung, Projekte in zwei Spalten mit Tagessumme h:mm. Das laufende Projekt zählt die laufende Zeit live dazu.
- **Projektbestätigung:** Fläche in Projektfarbe, darauf der Name (Font 4) und darunter groß (Font 6) die **aufsummierte Zeit dieses Projekts heute** als h:mm (`--:--`, solange die Uhrzeit unbekannt ist). Die Summen beginnen mit jedem Tag bei 0 (`accrue()` teilt an der lokalen Mitternacht und behält nur den heutigen Tag); beim Wechsel zählt die gerade beginnende Strecke noch nicht mit. Die Anzeige bleibt `PROJECT_SHOW_MS` = 5 s. Der Stick bleibt so lange wach; ein weiterer Wechsel startet die Zeit neu. Die Tagesübersicht (Seite 0, Taste A) bleibt `STATUS_SHOW_MS` = 8 s. Mehr Anzeigezeit kostet Akku (Display und Backlight, nicht gemessen).
- **Töne:** Projekt p: ein weicher Einzelton mit der Frequenz `PROJECT_TONES_HZ[p]` (C-Dur-Pentatonik aufsteigend: 1 = 523 Hz, 2 = 587, 3 = 659, 4 = 784, 5 = 880, 6 = 1047, 7 = 1175, 8 = 1319; 9 bis 12 nur für Prisma und 12-Eck-Münze). Pause (Seite 0 und „unbekannt“) = 392 Hz. Der Ton wird in `ui.cpp` selbst erzeugt, weil `M5.Speaker.tone()` nur Rechteck kann: Sinus plus leise Oktave, 8 ms Anschwung, Abklingen (Zeitkonstante 110 ms), 300 ms lang, 16 Bit mono mit 22050 Hz über `M5.Speaker.playRaw()` (Puffer 13 KB RAM). Er läuft im Hintergrund, während die Anzeige aufgebaut wird; `commit()` wartet am Ende auf das Tonende, damit der Stick nicht mitten im Ton einschläft. Im Teach-in bestätigt der Ton der erfassten Seite (Seite 0 = Pause-Ton). Daneben gibt es zwei Rechtecktöne: Langer Ton (1800 Hz, 350 ms) = Teach-in Start/Ende, WLAN-Export bereit. Drei tiefe Töne (800 Hz) = Fehler oder Abbruch. **Vom Nutzer bestätigt:** Die Töne sind gut, Lautstärke und Klang passen (Amplitude ca. 0,7 der Vollaussteuerung bei `SPEAKER_VOLUME` 140); die Töne sind nur in `config.h` (Tabelle) und `ui.cpp` (Klang) einstellbar. **Anlaufzeit des Lautsprechers:** Der Lautsprecher des StickS3 besteht aus dem Codec ES8311 und einem Verstärker. M5Unified schaltet beides erst in `M5.Speaker.begin()` ein (PM1-GPIO3 und eine I2C-Initialisierung des Codecs, aus `M5Unified.inl` gelesen). Ohne Vorstart läuft `begin()` erst mit dem Ton des ersten Wechsels, und der erste Ton nach dem Start geht vermutlich in der Anlaufzeit verloren (nicht gemessen; beim Rollen im selben Wachabschnitt läuft der Lautsprecher schon). Gegenmaßnahme: `ui::prepareSound()` startet den Lautsprecher beim Wecken per Bewegung gleich nach dem Boot, und ein Ton, der weniger als 200 ms (`kWarmupMs`) nach dem Start gespielt wird, beginnt mit Stille (der Puffer wächst dafür auf 22 KB). Am Gerät bestätigt: Der Ton beim Wecken aus dem Liegen in ein Projekt kommt. Welcher der beiden Teile (Vorstart oder Stille) den Ausschlag gibt, ist nicht getrennt geprüft. Fehlt der Ton wieder, `kWarmupMs` erhöhen. Der Vorstart blockiert den Boot kurz; seriell steht `Lautsprecher vorgestartet in <ms> ms` (am Gerät 5 ms).
- **Rotation:** Status und Texte quer (`DISP_ROT_FLAT` = 1). Münze aufgestellt: `(DISP_ROT_OFFSET + DISP_ROT_DIR × Quadrant) & 3`, der Quadrant ist die Lage in der Displayebene in 90°-Schritten.

#### WLAN-Export
Taste A 2 s halten und loslassen: WLAN verbinden (8 s), nebenbei NTP, Webserver auf Port 80 und mDNS `zeitprisma.local`. Das Display zeigt „WLAN-Export / http://zeitprisma.local / IP“, dazu ein langer Ton. Seite `/` zeigt Tagessummen, aktives Projekt, Akku, die Größe aller Logs und die Liste der Tagesdateien (neueste zuerst, höchstens 60). `/tag?d=JJJJ-MM-TT` lädt einen Tag herunter (auch `d=ohne_zeit` und `d=alt`, das Argument wird streng geprüft), `/log.csv` lädt alle Logs hintereinander mit einer Kopfzeile als `zeitprisma.csv` herunter (mit `Content-Length`, bewusst nicht „chunked“: Die chunked-Übertragung hing am Gerät, `curl` meldete „Malformed encoding found in chunked-encoding“ bzw. blieb nach dem ersten Block stehen, mit `--http1.0` kam die Datei korrekt). Abruf vom Rechner: `curl http://zeitprisma.local/tag?d=2026-10-29`, wenn der Rechner im selben WLAN ist und `zeitprisma.local` auflöst (sonst die IP vom Display nehmen). Ende: Taste A klicken oder nach 5 min. Ohne WLAN: „keine Verbindung“ und drei tiefe Töne.

#### Log-Datei
Eine Datei je Tag auf LittleFS: `/log_JJJJ-MM-TT.csv` (lokales Datum des Wechsels), Kopf `zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok`, eine Zeile je übernommenem Wechsel. `zeit` ist die lokale ISO-Zeit (leer, wenn die Uhrzeit nicht gültig war, dann `zeit_ok` = 0), `epoch` Sekunden, `seite` −1 bei unbekannter Lage, `projekt` 0 = Pause, `name` ohne Komma und Anführungszeichen, `vbat_mv` Akkuspannung. Wechsel ohne gültige Uhrzeit stehen in `/log_ohne_zeit.csv`.
- **Tagessummen im Gerät:** Die Summen auf dem Display (RTC-RAM) beginnen jeden Tag bei 0 (lokale Mitternacht), die Logs bleiben. Aus ihnen lassen sich die Tage später auswerten.
- **Übertragszeile:** Ist die Tagesdatei neu und lief das vorige Projekt schon vor Mitternacht, steht nach dem Kopf der Zustand um `00:00:00` (Projekt des Vortags, `vbat_mv` = 0). So ist jede Tagesdatei für sich auswertbar; in der Gesamtdatei ändert die Zeile die Summen nicht (gleiches Projekt, `auswertung.py` teilt Abschnitte ohnehin an Mitternacht). Tage ohne Wechsel haben keine Datei: Läuft ein Projekt mehrere Tage durch, steht die Übertragszeile erst im Tag des nächsten Wechsels.
- **Alt-Datei:** Eine vorhandene `/log.csv` (Format der älteren Firmware) wird nicht mehr beschrieben, im Export aber weiter angeboten (Schlüssel `alt`, in der Gesamtdatei zuerst).
- **Export:** siehe WLAN-Export. Einzelne Tage lassen sich auch aneinanderhängen (`cat log_*.csv > alle.csv`); zusätzliche Kopfzeilen überspringt `auswertung.py`, weil dort `zeit_ok` nicht „1“ ist.
- **Platz:** Die LittleFS-Partition ist 1,5 MB groß (`default_8MB.csv`, 0x180000). Rechnerisch (nicht gemessen) belegt jede Datei mindestens einen 4-KB-Block, das reicht für etwa 380 Tagesdateien. Ist der Speicher voll, scheitert das Schreiben (`Log schreiben fehlgeschlagen`). Alte Tage löscht die Firmware nicht.
- **Am Gerät geprüft (per WLAN-Export abgerufen):** Tagesdatei und Alt-Datei sind einzeln korrekt lesbar (Kopf + Wechsel, alle `zeit_ok` = 1, keine Übertragszeile, weil kein Mitternachtsübergang). `/log.csv` per HTTP/1.1 kam 3 von 3 Mal vollständig an (Länge = `Content-Length`, alle drei identisch, genau Kopfzeile + Alt-Datei ohne Kopf + Tagesdatei ohne Kopf; die Datei wächst nur am Ende, ein früherer Stand ist jeweils der Anfang des späteren). 24 weitere Abrufe aller vier Adressen (`/`, `/log.csv`, `/tag?d=<Tag>`, `/tag?d=alt`) lieferten alle 200 in 0,08 bis 0,36 s. Ungültige Argumente liefern 400 (`../../etc/passwd`, `2026-10-2x`), eine fehlende Datei 404. `tools/auswertung.py` rechnet die Gesamtdatei durch. Einmal lief ein `curl` auf einen Einzeltag-Abruf in ein Zeitlimit (28), danach nicht reproduzierbar, Ursache nicht geklärt. Der Tageswechsel ist geprüft: Die Übertragszeile steht um `00:00:00` am Anfang der neuen Tagesdatei, die Datei wurde mit dem ersten Wechsel des Tages angelegt (`size()`-Prüfung). Am PC getestet sind nur Dateiname und lokale Mitternacht (auch am Tag der Zeitumstellung).

#### Serielle Ausgabe (USB-CDC, Log-Level Info)
Eigene Ausgaben der Firmware:
- `Start: kalt=<1|0> Weckgrund=<n> Seite=<Seite> Modell=<1|0> Akku=<mV> mV Bauform=Muenze/9 | Boot ms: setup@<ms> begin=<ms> ui=<ms> hal=<ms> store=<ms> imu=<eigen|M5>`. **Weckgrund** (ESP-IDF): 0 = kein Aufwachen aus Deep Sleep (Einschalten, Reset, Flashen), 2 = Bewegung (G13), 3 = Taste A, 4 = Timer. `Seite` −1 = Pause oder unbekannt. `Muenze/9` = Münze mit 8 Randseiten plus Seite 0, beim Prisma `Prisma/10`. **Boot ms** (in ms, `millis()`): `setup@` = Chip-Start bis `setup()`, `begin` = `M5.begin()`, `ui` = `ui::begin()`, `hal` = `hal::begin()` (bei M5-IMU-Init mit Abgleich), `store` = LittleFS und Modell laden, `imu` = `eigen` (Warmstart ohne M5-IMU-Init) oder `M5`. Der Zeitstempel vorn in der Zeile ist genau diese Summe. Gemessene Werte: Warmstart `setup@64 begin=434 ui=0 hal=72 store=2..6 imu=eigen` (Summe 572..576), mit M5-Init `begin=664 hal=89 store=5 imu=M5` (822), Kaltstart `setup@247 begin=664 ui=1 hal=89 store=5` (1006).
- `IMU-Abgleich: ...` bei jedem Start mit M5-IMU-Init (siehe „Start“). Warnung `BMI270 nicht mehr initialisiert, M5-Init nachgeholt`, wenn der Schnellstart das BMI270 nicht initialisiert vorfand.
- `Seite <n> -> <Projektname>` bei jedem übernommenen Wechsel.
- `Schuetteln erkannt: Uebersicht` bzw. `ignoriert (nicht liegend, unruhig oder Anzeige aktiv)`.
- `[W] WebServer.cpp:425 send(): content length is zero`: Der Arduino-Core meldet das bei jeder Antwort ohne Textkörper, auch bei `streamFile` (jeder Tages-Download) und beim Kopf von `/log.csv`. Harmlos, beim Export 24 Mal gesehen.
- Nach dem Löschen des NVS (Neu-Einlernen) meldet `Preferences.cpp: begin(): nvs_open failed: NOT_FOUND` einmal, dass es den Namespace noch nicht gibt: harmlos, danach legt das Teach-in ihn an. Am Gerät gesehen.
- Warnungen und Fehler: `LittleFS leer/defekt -> formatiere`, `WLAN nicht erreichbar`, `Log schreiben fehlgeschlagen`, `M5PM1 nicht erreichbar`, `BMI270 nicht gefunden`, `Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s`.
- Im Teach-in steht nur je erfasster Seite und am Ende eine Zeile (siehe Teach-in), beim Warten nichts. Die WLAN-Zugangsdaten gibt die Firmware nirgends aus.
- Zeilenformat: `[ms seit Start][Level][Datei:Zeile] Funktion(): Text`. Am Gerät gesehen (nach Watchdog-Reset): dreimal `i2cInit(): Initialising I2C Master: sda=47 scl=48 freq=100000` (Arduino-Core), dann `setup(): Start: kalt=1 Weckgrund=0 Seite=-1 Modell=0 Akku=4170 mV Bauform=Muenze/9`, nach rund 6 s `WiFiGeneric.cpp ... Reason: 8 - ASSOC_LEAVE` (der Core meldet das eigene Abmelden nach dem NTP-Sync, kein Fehler). Fehlt „WLAN nicht erreichbar“, hat sich das WLAN verbunden. Ob NTP die Uhrzeit geliefert hat, steht nicht im Log (das Display zeigt „Heute …“ oder „Zeit unbekannt“).
- Nach dem Flashen und nach einem Watchdog-Reset ist `kalt=1` (der RTC-Zustand ist weg). Das Seitenmodell im NVS bleibt dabei erhalten (`Modell=1`, nach dem Flashen am Gerät gesehen). `Modell=0` und damit ein Teach-in gibt es beim allerersten Start, nach dem Löschen des NVS und wenn sich die Bauform ändert (die Modell-Version enthält die Seitenzahl).

### Fehlersuche

| Beobachtung | Bedeutung und Maßnahme |
|---|---|
| Stick taucht alle 1–2 s am USB auf und verschwindet | Wecken-Schleife (Deep Sleep, sofort wieder geweckt). Bekannte Ursache: INT1 des BMI270 pulst (`INT_MAP_DATA`, siehe Diagnose der Weckkette); die Firmware verhindert das beim Armieren. Kommt es wieder: `WAKE_DIAG 1` setzen und `DIAG Trace` auswerten. Zum Flashen Download-Modus erzwingen. |
| Nach dem Flashen bleibt der Port stabil, aber es kommt kein Log und das Display tut nichts | Stick steht noch im Download-Modus (ROM meldet `boot:0x21`). Watchdog-Reset auslösen (siehe Flashen). Kontrolle: `esptool ... --before no_reset chip_id` antwortet danach nicht mehr. |
| Port wechselt von `ttyACM0` auf `ttyACM1`, esptool meldet „Write timeout“ oder „port busy“ | Ein anderer Prozess hält den alten Port offen, meist ModemManager (`fuser -v /dev/ttyACM*`). Siehe ModemManager im Abschnitt Flashen. „Write timeout“ ohne Bootloader deutet darauf hin, dass die App läuft und nichts liest (so einmal beobachtet). |
| Schütteln im Liegen zeigt nichts | Im Log nachsehen: Kommt `Weckgrund=2` mit `Seite=0` und danach `Schuetteln erkannt: Uebersicht`, ist alles in Ordnung. Fehlt der Wecker, war die Bewegung zu sanft für any-motion (`ANYMOT_THRESHOLD` senken). Kommt `ignoriert`, stimmt eine Bedingung nicht (nicht liegend, unruhige Sitzungen, Anzeige aktiv). Kommt gar nichts, `SHAKE_JERK_G` senken oder kräftiger und länger schütteln (rund 0,5 s nach dem Boot). |
| Display „Zeit unbekannt“ | WLAN oder NTP fehlgeschlagen. `config_local.h` prüfen (SSID, Passwort, nur 2,4-GHz-Netz), seriell „WLAN nicht erreichbar“. |
| „Hardwarefehler / PM1/IMU pruefen“ | M5PM1 oder BMI270 nicht per I2C erreichbar. I2C nur über `M5.In_I2C` nutzen. |
| Teach-in „abgebrochen (Zeit)“ oder „Reihenfolge falsch?“ | Siehe Teach-in. Taste A 6 s halten startet es neu. |
| Seite wird nicht erkannt, springt oder kommt zu spät | Der Stick muss ca. in der Drehachse stehen (bis 35° aus der Ebene gekippt). Die Erkennung dauert rund 0,4 s nach dem Stillstand (`STILL_WINDOW` + `SETTLE_MS`), nach dem Wecken aus dem Deep Sleep kommt der Start (0,57 s bis zur Startzeile, dazu ROM und Bootloader, ungemessen) dazu. Springt es an einer Seitengrenze: `RING_HYST_DEG` erhöhen. Werden beim Rollen Zwischenseiten mitgezählt: `SETTLE_MS` erhöhen. Sonst Teach-in neu. |
| `IMU-Abgleich: ... NICHT erlaubt` im Log | Der Stick lag beim Start nicht ruhig (|g| außerhalb 0,8 bis 1,2) oder die Achsen stimmen nicht mit M5Unified überein (Abweichung ≥ 0,08 g). Bei Bewegung unkritisch: der nächste Start prüft erneut und bleibt bis dahin beim M5-Init (`imu=M5`). Bleibt es bei ruhigem Stick `NICHT erlaubt`, passt die Achsenabbildung in `readBmiAccel()` nicht, dann den Schnellstart in `main.cpp` abschalten (`ownImu = false`). |
| `E (n) I2S: i2s_driver_uninstall(2048): I2S port 0 has not installed` beim Einschlafen | Harmlos. `hal.cpp` ruft vor dem Schlafen `M5.Speaker.end()`, auch wenn in diesem Wachabschnitt kein Ton lief und der Lautsprecher nie gestartet wurde. Erscheint nur in Wachabschnitten ohne Wechsel. |
| Nach dem Flashen kommt wieder das Teach-in | Erwartet nur, wenn sich die Bauform (Env) geändert hat oder das NVS gelöscht wurde: Die Modell-Version enthält die Seitenzahl. Beim Flashen derselben Bauform bleibt das Modell erhalten (am Gerät gesehen, `Modell=1`), gelöscht wird nur der RTC-Zustand (Kaltstart, `kalt=1`). |

### Strom
`POWER_DEEPSLEEP 1`: Bewegung weckt den ESP32 über BMI270 any-motion → M5PM1 PYG4 → IRQ PYG1 → G13. Danach wird abgetastet, geloggt und wieder geschlafen. M5 gibt für diesen Board-Zustand (L2) ca. 102 µA an. Rechnerisch reicht das für einige Wochen pro Ladung, real bitte messen: `vbat_mv` wird mitgeloggt. Dauerwackeln (Tasche) → 5 min Timer-Schlaf. Rückfall `POWER_DEEPSLEEP 0`: Light Sleep mit Abfrage, einfacher, aber spürbar mehr Verbrauch.

### Offene Punkte (ungetestet)
- **Am Gerät nicht getestet:** Teach-in im Gehäuse (nicht gesondert gemeldet), Taste A 6 s halten (Teach-in per Taste), Darstellung eigener Projektnamen am Display, Anzeige-Rotation des aufgestellten Textes, Stromverbrauch, schnelles Rollen mit der kurzen Entprellung. Bestätigt sind Start, Wecken per Bewegung, Seitenerkennung beim Rollen von Hand, Töne, Projektanzeige mit Tageszeit, Schütteln im Liegen, Taste A kurz, der WLAN-Export und der Tageswechsel (siehe Stand der Prüfung).
- **Bootzeit, Rest (nicht untersucht):** Nach dem Schnellstart bleiben 0,57 s bis zur Startzeile: `setup@64`, `begin` 434 ms (`M5.begin()` ohne IMU: Display- und Power-Initialisierung, nicht aufgeschlüsselt), `hal` 72 ms. Ob sich die Display-Initialisierung beim Wecken per Bewegung sparen lässt (das Display bleibt dort ohnehin aus), ist offen.
- **Wecken per Bewegung:** Nicht untersucht: wie viele Wecken ein Dauerrollen auslöst und wie sich Bewegung in einer Tasche verhält (45-s-Sitzungsgrenze, dann 5 min Schlaf).
- **Weckkette, Empfindlichkeit:** Die any-motion-Konfiguration ist am Gerät zurückgelesen und stimmt (Dauer 5, Schwelle 0xAA ca. 80 mg, alle Achsen, nur INT1), und echte Bewegung weckt. Ob die Schwelle für sanftes Drehen der Münze reicht und ob Tischerschütterungen ungewollt wecken, ist nach Gefühl im Alltag abzustimmen (`ANYMOT_THRESHOLD`, `ANYMOT_DURATION`).
- **Magnet-Halt:** Annahme „Magnetic back“ hält an Stahl. Der Stick sitzt nur magnetisch in der offenen Tasche, falls zu schwach: dünnes doppelseitiges Klebeband.
- **Display-Rotation:** `DISP_ROT_OFFSET/DIR` in `config.h` einmal am Gerät kalibrieren.
- **Geometrie:** Gehäusemaße aus dem offiziellen Strukturmodell (`M5_Hardware/Products/K150_StickS3`), Schwerpunkt des Sticks unbekannt (Annahme: Mitte), USB-Loch nur für schlanke Stecker.
- **Uhr im Deep Sleep:** Die Zeit läuft über die RTC des ESP32 weiter (die Zeitstempel im Log blieben über die Schlafphasen gültig). Ihre Genauigkeit über Stunden ist nicht gemessen; abgeglichen wird per NTP nur bei einem Wechsel, wenn der letzte Abgleich älter als 6 h ist (`RESYNC_SEC`).

// Zeitprisma – Einstellungen
#pragma once

// ---------------------------------------------------------------- Projekte
// Bauform per Build-Flag (platformio.ini): Standard = 8-Eck-Muenze,
// -DPRISMA_RING=12 = 12-Eck-Muenze, -DPRISMA_PRISMA10 = Zehnkant-Prisma.
// Index = Seite, die OBEN liegt. Seite 0 = Pause:
//   Muenze: auf dem Ruecken liegend (Display oben); Display unten zaehlt ebenfalls als Pause
//   Prisma: Display oben; aufrecht stehend zaehlt ebenfalls als Pause
#include "prisma_core.h"
// Eigene Einstellungen (WLAN-Zugangsdaten, Projektnamen) stehen in config_local.h. Die Datei steht in .gitignore
// (das Repo ist oeffentlich, Kundennamen und Passwoerter gehoeren nicht hinein), Vorlage: config_local.h.example.
// Was dort definiert ist, ersetzt die allgemeinen Werte hier.
#if __has_include("config_local.h")
#include "config_local.h"
#endif
#if defined(PROJECT_NAMES_LOCAL)
// Pause + Projekte aus config_local.h (mindestens kFaces Eintraege, mehr sind erlaubt)
static const char* const PROJECT_NAMES[] = {PROJECT_NAMES_LOCAL};
#elif defined(PRISMA_PRISMA10)
static const char* const PROJECT_NAMES[] = {
    "Pause", "Projekt 1", "Projekt 2", "Projekt 3", "Projekt 4",
    "Projekt 5", "Projekt 6", "Projekt 7", "Projekt 8", "Projekt 9",
};
#elif PRISMA_RING == 12
static const char* const PROJECT_NAMES[] = {
    "Pause", "Projekt 1", "Projekt 2", "Projekt 3", "Projekt 4", "Projekt 5", "Projekt 6",
    "Projekt 7", "Projekt 8", "Projekt 9", "Projekt 10", "Projekt 11", "Projekt 12",
};
#else  // 8 Randseiten
static const char* const PROJECT_NAMES[] = {
    "Pause", "Projekt 1", "Projekt 2", "Projekt 3", "Projekt 4",
    "Projekt 5", "Projekt 6", "Projekt 7", "Projekt 8",
};
#endif
// Seitenerkennung: Ringerkennung ohne Totzone, Parameter im Block "Erkennung" weiter unten.
// RGB565-Farben fuer die Bestaetigung am Display (Index = Projekt)
static const uint16_t PROJECT_COLORS[] = {
    0x7BEF,  // grau (Pause)
    0x041F,  // blau
    0x07E0,  // gruen
    0xFD20,  // orange
    0xF81F,  // magenta
    0x07FF,  // cyan
    0xFFE0,  // gelb
    0xF800,  // rot
    0xAFE5,  // hellgruen
    0xC618,  // hellgrau
    0x897F,  // violett
    0xFBEA,  // lachs
    0x2D7F,  // himmelblau
};

// Display-Drehung (M5GFX setRotation 0..3)
#define DISP_ROT_FLAT    1       // Statusanzeige (Display liegt oben)
// Muenze aufgestellt: Text automatisch aufrecht = (OFFSET + DIR * Quadrant) & 3.
// Die Zuordnung IMU-Achsen <-> Displayrotation ist nicht dokumentiert: einmal pruefen.
// Steht der Text mit der obersten Seite (8 bzw. 12) oben falsch: OFFSET aendern (0..3).
// Stimmt das, aber die seitlichen Lagen stehen auf dem Kopf: DIR = -1.
#define DISP_ROT_OFFSET  0
#define DISP_ROT_DIR     1

// Anzeigedauer: so lange zeigt der Stick etwas und bleibt wach (ein neuer Wechsel startet die Zeit neu)
#define PROJECT_SHOW_MS  5000    // Projektfarbe und Name nach einem Wechsel
#define STATUS_SHOW_MS   8000    // Tagesuebersicht (Seite 0, Taste A)

// Ton je Projekt in Hz (Index = Projekt, 0 = Pause): C-Dur-Pentatonik, aufsteigend. Jedes Projekt hat einen eigenen
// weichen Einzelton (Sinus mit leiser Oktave und Abklingen, ui.cpp); die Pause ist die tiefste Note.
// Ab Index 9 nur fuer die 12-Eck-Muenze und das Prisma (9 Projekte).
static const uint16_t PROJECT_TONES_HZ[] = {
    392,   // Pause  G4
    523,   // 1      C5
    587,   // 2      D5
    659,   // 3      E5
    784,   // 4      G5
    880,   // 5      A5
    1047,  // 6      C6
    1175,  // 7      D6
    1319,  // 8      E6
    1568,  // 9      G6
    1760,  // 10     A6
    2093,  // 11     C7
    2349,  // 12     D7
};

// ---------------------------------------------------------------- WLAN / Zeit
// Zugangsdaten NIE hier eintragen (diese Datei ist in git, das Repo oeffentlich):
// sie stehen in config_local.h (siehe oben, Vorlage: config_local.h.example).
#ifndef WIFI_SSID
#define WIFI_SSID        "dein-wlan"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS        "dein-passwort"
#endif
#define TZ_INFO          "CET-1CEST,M3.5.0,M10.5.0/3"   // Wien
#define NTP_1            "at.pool.ntp.org"
#define NTP_2            "pool.ntp.org"
#define RESYNC_SEC       (6UL * 3600UL)   // bei Ereignis neu syncen, wenn letzter Sync aelter
#define WIFI_TIMEOUT_MS  8000
#define MDNS_NAME        "zeitprisma"      // http://zeitprisma.local
#define WEB_TIMEOUT_MS   (5UL * 60UL * 1000UL)

// ---------------------------------------------------------------- Erkennung
#define SAMPLE_MS        20      // 50 Hz
#define STILL_WINDOW     6       // 6 Samples = 0,12 s ruhig, bevor eine Seite gezaehlt wird
#define STILL_DEV_DEG    3.0f    // max. Abweichung im Fenster
#define SETTLE_MS        250     // so lange muss eine neue Seite danach ruhig liegen (Wechsel ca. 0,4 s nach Stillstand)
#define TEACH_WINDOW     50      // Teach-in: 50 Samples = 1 s ruhig je Seite
#define SESSION_MAX_MS   45000   // spaetestens dann wieder schlafen (z. B. in der Tasche)
// Schuetteln (nur liegend, d. h. Seite 0 uebernommen): zeigt die Tagesuebersicht, auch wenn sich die Seite nicht aendert.
// Eine Spitze ist eine Aenderung zwischen zwei Samples (20 ms) ueber SHAKE_JERK_G; SHAKE_PEAKS Spitzen innerhalb
// SHAKE_WINDOW_MS gelten als Schuetteln (prisma_core.h, ShakeDetector). Bis zur Erkennung dauert es ca. 0,5 s Schuetteln.
#define SHAKE_JERK_G     0.25f
#define SHAKE_PEAKS      4
#define SHAKE_WINDOW_MS  1000
#define SHAKE_GRACE_MS   2000    // nach dem Wecken im Liegen so lange wach bleiben, damit ein Schuetteln erkannt wird
// Ringerkennung (prisma_core.h, classifyRing): Steht der Stick ca. in der Drehachse (die Beschleunigung
// liegt ca. in der Ringebene), wird immer die naechste Seite erkannt, auch zwischen zwei Seiten.
// Die Hysterese verhindert Flackern an den Seitengrenzen. Standardwerte kommen aus prisma_core.h.
#define RING_HYST_DEG         (prisma::kRingHystFrac * prisma::kStepDeg)  // 8 Seiten: 9 Grad
#define RING_UPRIGHT_DEG      prisma::kUprightDeg      // Eintritt: hoechstens 35 Grad aus der Ringebene
#define RING_UPRIGHT_EXIT_DEG prisma::kUprightExitDeg  // Verbleib: bis 45 Grad
#define FLAT_DEG              prisma::kFlatDeg         // Muenze auf dem Ruecken (Seite 0): bis 30 Grad von "Display oben"
#define FLAT_EXIT_DEG         prisma::kFlatExitDeg     // ... behalten bis 40 Grad

// ---------------------------------------------------------------- Strom
// 1 = Deep Sleep, Aufwachen per IMU-Bewegungsinterrupt ueber M5PM1 (sparsam)
// 0 = Light Sleep mit Abfrage alle LIGHT_POLL_MS (einfacher, mehr Verbrauch)
#define POWER_DEEPSLEEP  1
#define LIGHT_POLL_MS    700
// BMI270 any-motion: 1 LSB = 0,48 mg bzw. 20 ms (Bosch-Doku / M5-Beispiel)
#define ANYMOT_THRESHOLD 0xAA    // ~83 mg (Bosch-Default)
#define ANYMOT_DURATION  5       // 100 ms
// Nach dem Armieren schwingt das BMI270 ein und loest einmal aus (gemessen ca. 130 ms danach): wach bleiben,
// den M5PM1-IRQ bei jeder Ausloesung loeschen und erst schlafen, wenn G13 IRQ_QUIET_MS lang ruhig HIGH war.
#define IRQ_QUIET_MS      300    // so lange muss G13 ruhig HIGH sein
#define IRQ_SETTLE_MAX_MS 2500   // laenger nicht warten (dann Timer-Rueckfall 60 s)
#define SPEAKER_VOLUME   140     // < 75 % auf Akku (M5-Hinweis)
// Diagnose der Weckkette (nur zur Fehlersuche, kostet Strom, Standard 0): 1 = beim Wecken die IRQ-Quellen loggen
// (M5PM1- und BMI270-Status), nach dem Armieren das BMI270 zurueckpruefen, INT1 und G13 anderthalb Sekunden mit
// Mikrosekunden-Zeitstempeln verfolgen und danach nochmal anderthalb Sekunden die Bewegung (mg) loggen.
#define WAKE_DIAG        0

// ---------------------------------------------------------------- Bedienung Taste A
#define BTN_WEB_MS       2000    // halten: WLAN-Export starten
#define BTN_TEACH_MS     6000    // halten: Teach-in starten

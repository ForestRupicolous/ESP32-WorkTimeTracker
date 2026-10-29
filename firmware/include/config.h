// Zeitprisma – Einstellungen
#pragma once

// ---------------------------------------------------------------- Projekte
// Bauform per Build-Flag (platformio.ini): Standard = 8-Eck-Muenze,
// -DPRISMA_RING=12 = 12-Eck-Muenze, -DPRISMA_PRISMA10 = Zehnkant-Prisma.
// Index = Seite, die OBEN liegt. Seite 0 = Pause:
//   Muenze: auf dem Ruecken liegend (Display oben); Display unten zaehlt ebenfalls als Pause
//   Prisma: Display oben; aufrecht stehend zaehlt ebenfalls als Pause
#include "prisma_core.h"
#if defined(PRISMA_PRISMA10)
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
// Fangbereich um jede Seite: 40 % des Winkels zwischen Nachbarseiten
// (8 Seiten: +-18 Grad, 12 Seiten: +-12 Grad, Prisma 10: +-14,4 Grad)
#define MATCH_DEG        (0.4f * prisma::kStepDeg)
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

// ---------------------------------------------------------------- WLAN / Zeit
#define WIFI_SSID        "dein-wlan"
#define WIFI_PASS        "dein-passwort"
#define TZ_INFO          "CET-1CEST,M3.5.0,M10.5.0/3"   // Wien
#define NTP_1            "at.pool.ntp.org"
#define NTP_2            "pool.ntp.org"
#define RESYNC_SEC       (6UL * 3600UL)   // bei Ereignis neu syncen, wenn letzter Sync aelter
#define WIFI_TIMEOUT_MS  8000
#define MDNS_NAME        "zeitprisma"      // http://zeitprisma.local
#define WEB_TIMEOUT_MS   (5UL * 60UL * 1000UL)

// ---------------------------------------------------------------- Erkennung
#define SAMPLE_MS        20      // 50 Hz
#define STILL_WINDOW     25      // 25 Samples = 0,5 s
#define STILL_DEV_DEG    3.0f    // max. Abweichung im Fenster
#define SETTLE_MS        3000    // so lange muss eine neue Seite ruhig liegen
#define SESSION_MAX_MS   45000   // spaetestens dann wieder schlafen (z. B. in der Tasche)

// ---------------------------------------------------------------- Strom
// 1 = Deep Sleep, Aufwachen per IMU-Bewegungsinterrupt ueber M5PM1 (sparsam)
// 0 = Light Sleep mit Abfrage alle LIGHT_POLL_MS (einfacher, mehr Verbrauch)
#define POWER_DEEPSLEEP  1
#define LIGHT_POLL_MS    700
// BMI270 any-motion: 1 LSB = 0,48 mg bzw. 20 ms (Bosch-Doku / M5-Beispiel)
#define ANYMOT_THRESHOLD 0xAA    // ~83 mg (Bosch-Default)
#define ANYMOT_DURATION  5       // 100 ms
#define SPEAKER_VOLUME   140     // < 75 % auf Akku (M5-Hinweis)

// ---------------------------------------------------------------- Bedienung Taste A
#define BTN_WEB_MS       2000    // halten: WLAN-Export starten
#define BTN_TEACH_MS     6000    // halten: Teach-in starten

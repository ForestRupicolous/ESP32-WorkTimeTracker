// Zeitprisma – Einstellungen
#pragma once

// ---------------------------------------------------------------- Projekte
// Index = Seite, die OBEN liegt. Seite 0 = Display oben = Pause/Status.
// Unbekannte Lage (z. B. aufrecht stehend) zaehlt ebenfalls als Pause.
static const char* const PROJECT_NAMES[6] = {
    "Pause",
    "Projekt 1",
    "Projekt 2",
    "Projekt 3",
    "Projekt 4",
    "Projekt 5",
};
// RGB565-Farben fuer die Bestaetigung am Display
static const uint16_t PROJECT_COLORS[6] = {
    0x7BEF,  // grau
    0x041F,  // blau
    0x07E0,  // gruen
    0xFD20,  // orange
    0xF81F,  // magenta
    0x07FF,  // cyan
};

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
#define MATCH_DEG        25.0f   // Seiten liegen 60 Grad auseinander -> +-25 Grad Fangbereich
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

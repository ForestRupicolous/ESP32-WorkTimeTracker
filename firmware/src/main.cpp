// Zeitprisma – Projektzeit-Wuerfel mit M5Stack StickS3
//
// Ablauf (POWER_DEEPSLEEP=1):
//   Bewegung weckt den ESP32 -> Lage abtasten -> neue Seite 3 s ruhig? -> Ereignis loggen
//   -> wieder Deep Sleep. Zustand ueberlebt im RTC-RAM, Zeit laeuft in der RTC weiter.
//
// Bedienung:
//   Oben liegende Seite = aktives Projekt. Display oben (Seite 0) oder aufrecht = Pause.
//   Taste A kurz: Tagesuebersicht | 2 s halten: WLAN-Export | 6 s halten: Teach-in
#include <M5Unified.h>

#include "config.h"
#include "hal.h"
#include "net.h"
#include "prisma_core.h"
#include "store.h"
#include "ui.h"

using namespace prisma;

// ---------------------------------------------------------------- Zustand im RTC-RAM
struct Persist {
  uint32_t magic;
  int8_t face;        // uebernommene Seite, kUnknown = Pause
  time_t since;       // Beginn des laufenden Abschnitts
  time_t lastSync;
  bool timeValid;
  uint8_t restless;   // Sitzungen in Folge ohne Ruhe (Tasche, Transport)
  DayTotals today;
};
RTC_DATA_ATTR static Persist g;
static constexpr uint32_t kMagic = 0x5A50524D;    // "ZPRM"
static constexpr time_t kEpochSane = 1704067200;  // 2024-01-01

static FaceModel model;
static StillDetector still(STILL_WINDOW, STILL_DEV_DEG);
static Tracker tracker(SETTLE_MS);
static uint32_t sessionStart = 0;
static uint32_t uiUntil = 0;  // solange etwas angezeigt wird, nicht schlafen

static time_t nowT() { return time(nullptr); }

static void maybeSync(bool force) {
  if (!force && g.timeValid && nowT() - g.lastSync < (time_t)RESYNC_SEC) return;
  if (net::syncTime()) {
    g.timeValid = true;
    g.lastSync = nowT();
  }
}

static void showStatusNow(uint32_t ms = 8000) {
  ui::showStatus(g.today, projectForFace(g.face), g.since, nowT(), g.timeValid, hal::batteryMv());
  uiUntil = millis() + ms;
}

// ---------------------------------------------------------------- Ereignis
static void commit(int face) {
  time_t now = nowT();
  if (g.timeValid && g.since > kEpochSane) accrue(g.today, projectForFace(g.face), g.since, now);
  g.face = (int8_t)face;
  g.since = now;
  int p = projectForFace(face);
  if (!store::appendEvent(now, g.timeValid, face, p, PROJECT_NAMES[p], hal::batteryMv()))
    log_e("Log schreiben fehlgeschlagen");
  log_i("Seite %d -> %s", face, PROJECT_NAMES[p]);

  if (p == 0) ui::beep(1, 1500, 140); else ui::beep(p);
  if (face == 0) {
    showStatusNow();
  } else if (face != 3) {  // bei Seite 3 oben liegt das Display auf dem Tisch
    ui::showProject(p, PROJECT_NAMES[p]);
    uiUntil = millis() + 2500;
  }
  bool wasValid = g.timeValid;
  maybeSync(false);
  if (!wasValid && g.timeValid) g.since = nowT();  // erst ab jetzt sinnvoll zaehlbar
}

// ---------------------------------------------------------------- Teach-in
static void teachIn() {
  TeachIn t;
  t.start();
  StillDetector sd(STILL_WINDOW * 2, STILL_DEV_DEG);  // 1 s ruhig
  ui::showText("Teach-in", "Display nach oben legen", "und ruhig liegen lassen");
  ui::longBeep();
  uint32_t t0 = millis();
  while (!t.done() && millis() - t0 < 180000) {
    Vec3 a;
    if (hal::readAccel(a)) sd.add(a);
    if (sd.still() && t.offer(sd.mean())) {
      int k = t.captured();
      ui::beep(k);
      char l2[32];
      snprintf(l2, sizeof l2, "Seite %d erfasst", k - 1);
      ui::showText("Teach-in", l2, k < kFaces ? "weiter: naechste Augenzahl oben" : "fertig");
      sd.reset();
      t0 = millis();  // Zeitfenster je Seite
    }
    delay(SAMPLE_MS);
  }
  if (!t.done()) {
    ui::showText("Teach-in", "abgebrochen (Zeit)");
    ui::beep(3, 800, 150);
    delay(2500);
    return;
  }
  float worst = 0;
  bool ok = t.plausible(15.f, &worst);
  char l3[40];
  snprintf(l3, sizeof l3, "Abweichung max %.0f Grad", worst);
  if (!ok) {
    ui::showText("Teach-in", "Reihenfolge falsch?", l3);
    ui::beep(3, 800, 150);
    delay(4000);
    return;  // altes Modell bleibt
  }
  model = t.result();
  store::saveModel(model);
  ui::showText("Teach-in OK", "gespeichert", l3);
  ui::longBeep();
  delay(2000);
  // Laufenden Abschnitt als Pause schliessen; die aktuelle Lage wird danach
  // regulaer (nach SETTLE_MS) als neues Ereignis uebernommen.
  if (g.face != kUnknown) commit(kUnknown);
  still.reset();
  tracker.restore(kUnknown);
}

// ---------------------------------------------------------------- WLAN-Export
static String statusHtml() {
  String h = F("<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width'>"
               "<title>Zeitprisma</title><body style='font-family:sans-serif;max-width:30em;margin:1em auto'>"
               "<h2>Zeitprisma</h2><table>");
  time_t now = nowT();
  char b[16];
  for (int p = 1; p < kFaces; ++p) {
    uint32_t s = isSameDay(g.today, now) ? g.today.sec[p] : 0;
    if (p == projectForFace(g.face) && g.timeValid) s += (uint32_t)(now - g.since);
    formatHM(b, sizeof b, s);
    h += "<tr><td>" + String(p) + "</td><td>" + PROJECT_NAMES[p] + "</td><td align=right>" + b + "</td></tr>";
  }
  char tail[200];
  snprintf(tail, sizeof tail,
           "</table><p>Aktiv: %s<br>Akku: %.2f V<br>Log: %u Bytes</p>"
           "<p><a href='/log.csv'>log.csv herunterladen</a></p>",
           PROJECT_NAMES[projectForFace(g.face)], hal::batteryMv() / 1000.f, (unsigned)store::logSize());
  h += tail;
  return h;
}

static void webExport() {
  ui::showText("WLAN-Export", "verbinde ...");
  bool stop = false;
  net::serveExport(
      statusHtml,
      [&]() {
        M5.update();
        if (M5.BtnA.wasClicked()) stop = true;
        return !stop;
      },
      [&](const char* ip) {
        if (!ip) {
          ui::showText("WLAN-Export", "keine Verbindung");
          ui::beep(3, 800, 150);
          delay(2500);
          return;
        }
        ui::showText("WLAN-Export", "http://" MDNS_NAME ".local", ip);
        ui::longBeep();
      });
  if (nowT() > kEpochSane && !g.timeValid) {  // Export hat nebenbei NTP geholt
    g.timeValid = true;
    g.lastSync = nowT();
    g.since = nowT();
  }
  showStatusNow(3000);
}

// ---------------------------------------------------------------- Schlafen
static void goToSleep(bool restless) {
  ui::off();
#if POWER_DEEPSLEEP
  if (restless && ++g.restless >= 3) hal::sleepFor(300);  // Dauerwackeln: 5 min Ruhe
  if (!restless) g.restless = 0;
  hal::sleepUntilMotion();
#else
  (void)restless;
  hal::imuLowPower();
  for (;;) {  // Light Sleep: nur kurz aufwachen und Lage pruefen
    hal::lightSleep(LIGHT_POLL_MS);
    if (hal::wokeByButton()) break;
    Vec3 a;
    if (!hal::readAccel(a)) continue;
    if (classify(model, a, MATCH_DEG) != tracker.committed()) break;
  }
  still.reset();
  tracker.restore(tracker.committed());
  sessionStart = millis();
  if (hal::wokeByButton()) showStatusNow();
#endif
}

// ---------------------------------------------------------------- Arduino
void setup() {
  auto cfg = M5.config();
  cfg.internal_mic = false;
  M5.begin(cfg);
  setenv("TZ", TZ_INFO, 1);  // TZ ueberlebt den Deep Sleep nicht
  tzset();
  ui::begin();

  bool cold = (g.magic != kMagic);
  if (cold) {
    memset(&g, 0, sizeof g);
    g.magic = kMagic;
    g.face = kUnknown;
    clearTotals(g.today);
  }
  if (!hal::begin()) ui::showText("Hardwarefehler", "PM1/IMU pruefen");
  store::begin();
  if (!store::loadModel(model)) model = defaultModel();
  tracker.restore(g.face);

  if (cold) {
    ui::showText("Zeitprisma", "hole Uhrzeit ...");
    maybeSync(true);
    if (!model.valid) teachIn();
    showStatusNow(5000);
  } else if (hal::wokeByButton()) {
    showStatusNow();
  } else {
    ui::off();  // Bewegung: Display aus lassen, bis es etwas zu zeigen gibt
  }
  sessionStart = millis();
}

void loop() {
  static uint32_t lastSample = 0;
  M5.update();

  // Taste A
  if (M5.BtnA.pressedFor(BTN_TEACH_MS)) {
    ui::showText("Loslassen:", "Teach-in");
    uiUntil = millis() + 2000;
  } else if (M5.BtnA.pressedFor(BTN_WEB_MS)) {
    ui::showText("Loslassen:", "WLAN-Export");
    uiUntil = millis() + 2000;
  }
  if (M5.BtnA.wasReleaseFor(BTN_TEACH_MS)) {
    teachIn();
  } else if (M5.BtnA.wasReleaseFor(BTN_WEB_MS)) {
    webExport();
  } else if (M5.BtnA.wasClicked()) {
    showStatusNow();
  }

  // Lage abtasten
  if (millis() - lastSample >= SAMPLE_MS) {
    lastSample = millis();
    Vec3 a;
    if (hal::readAccel(a)) still.add(a);
    bool isStill = still.still();
    int face = isStill ? classify(model, still.mean(), MATCH_DEG) : tracker.candidate();
    if (tracker.update(face, isStill, millis())) commit(tracker.committed());
  }

  // Schlafen?
  bool busy = (int32_t)(uiUntil - millis()) > 0 || M5.BtnA.isPressed();
  if (!busy && tracker.settled()) goToSleep(false);
  if (!busy && millis() - sessionStart > SESSION_MAX_MS) goToSleep(true);
}

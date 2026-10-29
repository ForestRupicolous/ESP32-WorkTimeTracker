// Zeitprisma – Projektzeit-Wuerfel mit M5Stack StickS3 (12-Eck-Muenze oder Zehnkant-Prisma)
//
// Ablauf (POWER_DEEPSLEEP=1):
//   Bewegung weckt den ESP32 -> Lage abtasten -> neue Seite SETTLE_MS ruhig? -> Ereignis loggen
//   -> wieder Deep Sleep. Zustand ueberlebt im RTC-RAM, Zeit laeuft in der RTC weiter.
//
// Bedienung:
//   Oben liegende Seite (Ziffer) = aktives Projekt.
//   Muenze: auf dem Ruecken (Display oben) oder Display unten = Pause.
//   Prisma: Display oben (Seite 0) oder aufrecht = Pause.
//   Taste A kurz: Tagesuebersicht | 2 s halten: WLAN-Export | 6 s halten: Teach-in
#include <M5Unified.h>

#include <utility>
#include <vector>

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
static constexpr uint32_t kMagic = 0x5A500300u | kFaces;  // Version 3 + Bauform
static_assert(sizeof(PROJECT_NAMES) / sizeof(PROJECT_NAMES[0]) >= kFaces,
              "zu wenige PROJECT_NAMES (config_local.h: PROJECT_NAMES_LOCAL braucht Pause + Projekte der Bauform)");
static_assert(sizeof(PROJECT_COLORS) / sizeof(PROJECT_COLORS[0]) >= kFaces, "zu wenige PROJECT_COLORS");
static_assert(sizeof(PROJECT_TONES_HZ) / sizeof(PROJECT_TONES_HZ[0]) >= kFaces, "zu wenige PROJECT_TONES_HZ");
static constexpr time_t kEpochSane = 1704067200;  // 2024-01-01

static FaceModel model;
static StillDetector still(STILL_WINDOW, STILL_DEV_DEG);
static Tracker tracker(SETTLE_MS);
static ShakeDetector shake(SHAKE_JERK_G, SHAKE_PEAKS, SHAKE_WINDOW_MS);
static const RingParams ringParams{RING_HYST_DEG, RING_UPRIGHT_DEG, RING_UPRIGHT_EXIT_DEG, FLAT_DEG,
                                   FLAT_EXIT_DEG};
static uint32_t sessionStart = 0;
static uint32_t uiUntil = 0;  // solange etwas angezeigt wird, nicht schlafen
static uint32_t awakeUntil = 0;  // Wachbleiben ohne Anzeige (Zeit zum Schuetteln nach dem Wecken im Liegen)

static time_t nowT() { return time(nullptr); }

// Muenze aufgestellt: Text aufrecht drehen; sonst feste Lage
static uint8_t rotationFor(int face, const Vec3& a) {
  if (!kCoin || face <= 0) return DISP_ROT_FLAT;
  return (uint8_t)((DISP_ROT_OFFSET + DISP_ROT_DIR * upQuadrant(a)) & 3);
}

static void maybeSync(bool force) {
  if (!force && g.timeValid && nowT() - g.lastSync < (time_t)RESYNC_SEC) return;
  if (net::syncTime()) {
    g.timeValid = true;
    g.lastSync = nowT();
  }
}

static void showStatusNow(uint32_t ms = STATUS_SHOW_MS) {
  ui::showStatus(g.today, projectForFace(g.face), g.since, nowT(), g.timeValid, hal::batteryMv());
  uiUntil = millis() + ms;
}

// ---------------------------------------------------------------- Ereignis
static void commit(int face, const Vec3& accel) {
  time_t now = nowT();
  // Abschnitt, der mit diesem Wechsel endet (fuer die Uebertragszeile, falls er ueber Mitternacht lief)
  const bool hadSegment = g.timeValid && g.since > kEpochSane;
  const store::Carry carry{g.since, g.face, projectForFace(g.face), PROJECT_NAMES[projectForFace(g.face)]};
  if (hadSegment) accrue(g.today, projectForFace(g.face), g.since, now);
  g.face = (int8_t)face;
  g.since = now;
  int p = projectForFace(face);
  if (!store::appendEvent(now, g.timeValid, face, p, PROJECT_NAMES[p], hal::batteryMv(), hadSegment ? &carry : nullptr))
    log_e("Log schreiben fehlgeschlagen");
  log_i("Seite %d -> %s", face, PROJECT_NAMES[p]);
  // aufsummierte Zeit des Projekts heute (die Summen beginnen mit jedem Tag bei 0); -1 = Uhrzeit unbekannt
  int32_t todaySec = -1;
  if (g.timeValid) todaySec = isSameDay(g.today, now) ? (int32_t)g.today.sec[p] : 0;

  ui::startProjectTone(p);  // der Ton laeuft im Hintergrund, waehrend die Anzeige aufgebaut wird
  if (face == 0) {
    showStatusNow();
  } else if (displayVisible(face)) {  // sonst zeigt das Display Richtung Tisch
    ui::showProject(p, PROJECT_NAMES[p], rotationFor(face, accel), todaySec);
    uiUntil = millis() + PROJECT_SHOW_MS;
  }
  bool wasValid = g.timeValid;
  maybeSync(false);
  if (!wasValid && g.timeValid) g.since = nowT();  // erst ab jetzt sinnvoll zaehlbar
  ui::waitTone();  // nicht mitten im Ton schlafen gehen
}

// ---------------------------------------------------------------- Teach-in
static void teachIn() {
  TeachIn t;
  t.start();
  StillDetector sd(TEACH_WINDOW, STILL_DEV_DEG);  // 1 s ruhig
  ui::setRotation(DISP_ROT_FLAT);
  ui::showText("Teach-in", "Display nach oben legen", "und ruhig liegen lassen");
  ui::longBeep();
  uint32_t t0 = millis();
  while (!t.done() && millis() - t0 < 180000) {
    Vec3 a;
    if (hal::readAccel(a)) sd.add(a);
    if (sd.still() && t.offer(sd.mean())) {
      int k = t.captured();
      log_i("Teach-in: Seite %d erfasst", k - 1);
      ui::beepProject(k - 1);
      ui::setRotation(rotationFor(k - 1, sd.mean()));
      char l2[32], l3[40];
      snprintf(l2, sizeof l2, "Seite %d erfasst", k - 1);
      if (k < kFaces) snprintf(l3, sizeof l3, "weiter: %d nach oben", k);
      else snprintf(l3, sizeof l3, "fertig");
      ui::showText("Teach-in", l2, l3);
      sd.reset();
      t0 = millis();  // Zeitfenster je Seite
    }
    delay(SAMPLE_MS);
  }
  if (!t.done()) {
    log_w("Teach-in: abgebrochen (Zeit), %d von %d Seiten erfasst", t.captured(), kFaces);
    ui::showText("Teach-in", "abgebrochen (Zeit)");
    ui::beep(3, 800, 150);
    delay(2500);
    return;
  }
  float worst = 0;
  bool ok = t.plausible(8.f, &worst);
  log_i("Teach-in: alle %d Seiten erfasst, Abweichung max %.1f Grad -> %s", kFaces, worst,
        ok ? "Modell gespeichert" : "verworfen (Reihenfolge falsch?)");
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
  if (g.face != kUnknown) commit(kUnknown, Vec3{0, 0, 1});
  still.reset();
  shake.reset();
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
           "</table><p>Aktiv: %s<br>Akku: %.2f V<br>Logs: %u Bytes</p>"
           "<p><a href='/log.csv'>Alle Tage (log.csv)</a></p><p>Ein Tag:<br>",
           PROJECT_NAMES[projectForFace(g.face)], hal::batteryMv() / 1000.f, (unsigned)store::logSize());
  h += tail;
  // neueste zuerst, hoechstens 60 (alle stehen in log.csv)
  std::vector<std::pair<String, size_t>> logs;
  store::listLogs([&](const char* key, size_t size) { logs.emplace_back(key, size); });
  int shown = 0;
  for (size_t i = logs.size(); i-- > 0 && shown < 60; ++shown) {
    h += "<a href='/tag?d=" + logs[i].first + "'>" + logs[i].first + "</a> " + String((unsigned)logs[i].second) + " B<br>";
  }
  h += "</p>";
  return h;
}

static void webExport() {
  ui::setRotation(DISP_ROT_FLAT);
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
    if (classifyRing(model, a, tracker.committed(), ringParams) != tracker.committed()) break;
  }
  still.reset();
  tracker.restore(tracker.committed());
  sessionStart = millis();
  if (hal::wokeByButton()) showStatusNow();
#endif
}

// ---------------------------------------------------------------- Arduino
void setup() {
  uint32_t tSetup = millis();  // Zeit von Chip-Start bis setup() (ohne ROM und Bootloader)
  bool cold = (g.magic != kMagic);
  // Wecken aus dem Deep Sleep: das BMI270 ist noch konfiguriert, die M5-IMU-Initialisierung (ca. 0,3 s) entfaellt
  bool ownImu = !cold && hal::warmImuReady();
  auto cfg = M5.config();
  cfg.internal_mic = false;
  cfg.internal_imu = !ownImu;
  M5.begin(cfg);
  uint32_t tBegin = millis();
  setenv("TZ", TZ_INFO, 1);  // TZ ueberlebt den Deep Sleep nicht
  tzset();
  ui::begin();
  uint32_t tUi = millis();

  if (cold) {
    memset(&g, 0, sizeof g);
    g.magic = kMagic;
    g.face = kUnknown;
    clearTotals(g.today);
  }
  if (!hal::begin(ownImu)) ui::showText("Hardwarefehler", "PM1/IMU pruefen");
  uint32_t tHal = millis();
  store::begin();
  if (!store::loadModel(model)) model = defaultModel();
  tracker.restore(g.face);
  uint32_t tStore = millis();
  log_i("Start: kalt=%d Weckgrund=%d Seite=%d Modell=%d Akku=%d mV Bauform=%s/%d | Boot ms: setup@%u begin=%u ui=%u hal=%u store=%u imu=%s",
        cold, (int)hal::wakeCause(), g.face, model.valid, hal::batteryMv(), kCoin ? "Muenze" : "Prisma", kFaces,
        (unsigned)tSetup, (unsigned)(tBegin - tSetup), (unsigned)(tUi - tBegin), (unsigned)(tHal - tUi),
        (unsigned)(tStore - tHal), ownImu ? "eigen" : "M5");

  if (cold) {
    ui::showText("Zeitprisma", "hole Uhrzeit ...");
    maybeSync(true);
    if (!model.valid) teachIn();
    showStatusNow(5000);
  } else if (hal::wokeByButton()) {
    showStatusNow();
  } else {
    ui::off();  // Bewegung: Display aus lassen, bis es etwas zu zeigen gibt
    // Lautsprecher frueh starten: er braucht eine Anlaufzeit, sonst geht der Ton des ersten Wechsels verloren
    if (hal::wakeCause() == ESP_SLEEP_WAKEUP_EXT0) {
      uint32_t tSnd = millis();
      ui::prepareSound();
      log_i("Lautsprecher vorgestartet in %u ms", (unsigned)(millis() - tSnd));
    }
    // Liegend (Pause): kurz wach bleiben, damit ein Schuetteln erkannt wird, auch wenn der Wecker vor dem Boot endete
    if (g.face == 0 && g.restless == 0) awakeUntil = millis() + SHAKE_GRACE_MS;
  }
  sessionStart = millis();
}

void loop() {
  static uint32_t lastSample = 0;
  M5.update();

  // Taste A
  if (M5.BtnA.pressedFor(BTN_WEB_MS)) ui::setRotation(DISP_ROT_FLAT);
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
    if (hal::readAccel(a)) {
      still.add(a);
      if (shake.add(a, millis())) {
        // nur liegend (Seite 0 uebernommen), nicht in der Tasche (unruhige Sitzungen) und nicht waehrend einer Anzeige
        bool show = tracker.committed() == 0 && g.restless == 0 && (int32_t)(uiUntil - millis()) <= 0;
        log_i("Schuetteln erkannt: %s", show ? "Uebersicht" : "ignoriert (nicht liegend, unruhig oder Anzeige aktiv)");
        if (show) showStatusNow();
      }
    }
    bool isStill = still.still();
    int face = isStill ? classifyRing(model, still.mean(), tracker.candidate(), ringParams) : tracker.candidate();
    if (tracker.update(face, isStill, millis())) commit(tracker.committed(), still.mean());
  }

  // Schlafen?
  bool busy = (int32_t)(uiUntil - millis()) > 0 || (int32_t)(awakeUntil - millis()) > 0 || M5.BtnA.isPressed();
  if (!busy && tracker.settled()) goToSleep(false);
  if (!busy && millis() - sessionStart > SESSION_MAX_MS) goToSleep(true);
}

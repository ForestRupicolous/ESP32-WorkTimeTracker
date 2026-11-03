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

#include <esp_timer.h>
#include <sys/time.h>

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
  time_t syncTryAt;   // Zeitpunkt des letzten erfolglosen Zeitabgleichs (Wiederholsperre SYNC_RETRY_SEC), 0 = keiner
  bool timeValid;
  uint8_t restless;   // Sitzungen in Folge ohne Ruhe (Tasche, Transport)
  DayTotals today;
  uint32_t noDateSec[kFaces];  // Summen je Projekt ohne gueltige Uhrzeit (Zeit seit dem Start, kein Tageswechsel)
  bool noDateOpen;    // im Log ohne Uhrzeit steht eine Neustart-Zeile ohne Ende-Zeile
};
RTC_DATA_ATTR static Persist g;
static constexpr uint32_t kMagic = 0x5A500500u | kFaces;  // Version 5 + Bauform
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

// Zeit seit dem Start bei Beginn eines Abgleichs ohne Uhrzeit (mono = 0: kein Abgleich ohne Uhrzeit unterwegs)
static struct {
  time_t rel;
  int64_t mono;
} relMark = {0, 0};

static time_t nowT() {
  const time_t t = time(nullptr);
  // Ohne Uhrzeit zaehlt die Zeit seit dem Start. Stellt der Abgleich im Hintergrund die Uhr, bevor die Schleife es
  // uebernommen hat, laeuft die Zeit seit dem Start bis dahin weiter (sonst waere ein Abschnitt Jahrzehnte lang)
  if (!g.timeValid && t > kEpochSane && relMark.mono != 0)
    return relMark.rel + (time_t)((esp_timer_get_time() - relMark.mono) / 1000000);
  return t;
}

// Uhrenvergleich vor und nach einem NTP-Abgleich: Springt die Systemuhr (die Uhr im Schlaf geht ungenau, im Log vom 30.10.2026
// ging sie ueber Nacht mehr als 4 Minuten vor), wird der laufende Abschnitt um den Sprung mitverschoben, sonst ginge seine
// Dauer verloren oder waere um den Sprung falsch. Die Zeitstempel kurz vor dem Abgleich bleiben, wie sie sind.
struct ClockMark {
  int64_t wallUs, monoUs;
};
static ClockMark clockMark() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return {(int64_t)tv.tv_sec * 1000000LL + tv.tv_usec, esp_timer_get_time()};
}

// Muenze aufgestellt: Text aufrecht drehen; sonst feste Lage
static uint8_t rotationFor(int face, const Vec3& a) {
  if (!kCoin || face <= 0) return DISP_ROT_FLAT;
  return (uint8_t)((DISP_ROT_OFFSET + DISP_ROT_DIR * upQuadrant(a)) & 3);
}

// Belegung des Log-Speichers in Prozent, einmal je Wachabschnitt gelesen (sie aendert sich langsam)
static int fsPercentNow() {
  static int cached = -1;
  if (cached < 0) {
    cached = store::usedPercent();
    if (cached >= FS_WARN_PERCENT) log_w("Speicher fast voll: %d %%", cached);
  }
  return cached;
}

// Summen, die gerade zaehlen (ohne den laufenden Abschnitt): heutige Summen, ohne Uhrzeit die Summen ohne Datum
static void currentTotals(uint32_t out[kFaces], time_t now) {
  if (!g.timeValid) {
    memcpy(out, g.noDateSec, kFaces * sizeof(uint32_t));
    return;
  }
  for (int p = 0; p < kFaces; ++p) out[p] = isSameDay(g.today, now) ? g.today.sec[p] : 0;
}

// Die Uhrzeit ist gerade gueltig geworden, nachdem Zeit ohne Datum gezaehlt wurde. Der Abschnitt ohne Datum endet bei
// endRel (Zeit seit dem Start), ein laufender Abschnitt geht ab jetzt mit Uhrzeit weiter. Beides steht im Log: eine
// Ende-Zeile im Log ohne Uhrzeit und, wenn ein Abschnitt lief, eine Fortsetzungszeile im Tageslog.
static void timeFound(time_t endRel) {
  const int p = projectForFace(g.face);
  const int vbat = hal::batteryMv();
  const bool running = g.since > 0;
  if (running) accrueRelative(g.noDateSec, p, g.since, endRel);
  store::appendEvent(endRel, false, kFaceTimeFound, 0, "Ende", vbat);
  g.timeValid = true;
  g.noDateOpen = false;
  if (running) {
    g.since = nowT();
    store::appendEvent(g.since, true, g.face, p, PROJECT_NAMES[p], vbat);
  }
  log_i("Uhrzeit gefunden, Abschnitt ohne Datum beendet (Summe Projekt %d: %lu s)", p, (unsigned long)g.noDateSec[p]);
}

// Nach einem Abgleich bei gueltiger Uhr: Sprung messen, protokollieren und den laufenden Abschnitt mitnehmen
static void followClockJump(const ClockMark& m0) {
  const ClockMark m1 = clockMark();
  const int64_t jumpMs = clockJumpMs(m0.wallUs, m1.wallUs, m0.monoUs, m1.monoUs);
  log_i("Zeit abgeglichen: Korrektur %+lld ms", (long long)jumpMs);
  const time_t jumpS = (time_t)(jumpMs / 1000);
  if (jumpS != 0 && g.since > kEpochSane) g.since += jumpS;
}

static bool statusShown = false;  // die Uebersicht ist gerade zu sehen (wird nachgezogen, wenn die Uhrzeit kommt)

static void showStatusNow(uint32_t ms = STATUS_SHOW_MS) {
  uint32_t sec[kFaces];
  currentTotals(sec, nowT());
  ui::showStatus(sec, projectForFace(g.face), g.since, nowT(), g.timeValid, hal::batteryMv(), fsPercentNow());
  uiUntil = millis() + ms;
  statusShown = true;
}

// ---------------------------------------------------------------- Zeitabgleich im Hintergrund
// Scan, Verbindung und NTP laufen in einer eigenen Task (net::syncStart); die Schleife tastet weiter ab und zaehlt weiter.
// pollSync() uebernimmt das Ergebnis: sofort, wenn die Uhr gestellt wurde, und am Ende der Task.
static bool syncPending = false;       // eine Task laeuft, ihr Ergebnis steht noch aus
static bool syncClockHandled = false;  // die gestellte Uhr ist schon uebernommen
static bool syncWasValid = false;      // die Uhr war beim Start des Abgleichs schon gueltig
static ClockMark syncM0;
static uint32_t syncStartMs = 0;
static constexpr uint32_t kSyncMaxMs = 60000;  // laenger darf die Task nicht wach halten

// Beginn der Zaehlung ohne Datum im Log ohne Uhrzeit festhalten (einmal je Zeitraum ohne Uhrzeit)
static void openNoDate() {
  if (g.noDateOpen) return;
  g.noDateOpen = true;
  store::appendEvent(nowT(), false, kFaceRestart, 0, "Neustart", hal::batteryMv());
}

static void startSync(bool force) {
  if (syncPending) return;
  const time_t now = nowT();
  if (!force) {
    if (g.timeValid && now - g.lastSync < (time_t)RESYNC_SEC) return;
    // nach einem erfolglosen Versuch nicht bei jedem Wechsel erneut: Scan und Verbindungsversuch kosten Sekunden
    if (g.syncTryAt > 0 && now >= g.syncTryAt && now - g.syncTryAt < (time_t)SYNC_RETRY_SEC) return;
  }
  syncWasValid = g.timeValid;
  syncClockHandled = false;
  syncM0 = clockMark();
  syncStartMs = millis();
  if (!g.timeValid) relMark = {time(nullptr), esp_timer_get_time() | 1};
  if (net::syncStart()) {
    syncPending = true;
  } else {
    relMark.mono = 0;
    g.syncTryAt = now;
  }
}

// Der SNTP-Dienst hat die Uhr gestellt: Uebergang ohne Datum -> mit Uhrzeit oder Sprung der Uhr mitnehmen
static void clockSetNow() {
  syncClockHandled = true;
  g.lastSync = time(nullptr);
  g.syncTryAt = 0;
  if (!syncWasValid) {
    if (g.since > 0 || g.noDateOpen) timeFound(nowT());  // Zeit ohne Datum lief
    else g.timeValid = true;                              // Kaltstart: noch nichts gezaehlt
  } else {
    followClockJump(syncM0);
  }
  relMark.mono = 0;
  if (statusShown && (int32_t)(uiUntil - millis()) > 0) showStatusNow(uiUntil - millis());  // "ohne Datum" nachziehen
}

// Die Task ist zu Ende (ok: Uhrzeit erhalten) oder wurde abgebrochen
static void finishSync(bool ok, bool cancelled) {
  syncPending = false;
  if (!syncClockHandled && (ok || (!syncWasValid && time(nullptr) > kEpochSane))) clockSetNow();
  relMark.mono = 0;
  if (!syncClockHandled && !cancelled) {
    g.syncTryAt = nowT();
    if (!g.timeValid) {
      log_w("Keine Uhrzeit gefunden: Zaehlung ohne Datum");
      openNoDate();
    }
  }
}

static void abortSync(bool cancelled) {
  if (!syncPending) return;
  net::syncCancel();
  bool ok = false;
  net::syncFinished(&ok);
  finishSync(ok, cancelled);
}

static void pollSync() {
  if (!syncPending) return;
  if (!syncClockHandled && net::takeClockSet()) clockSetNow();
  bool ok;
  if (net::syncFinished(&ok)) finishSync(ok, false);
  else if (millis() - syncStartMs > kSyncMaxMs) abortSync(false);
}

// ---------------------------------------------------------------- Ereignis
static void commit(int face, const Vec3& accel) {
  pollSync();  // eine inzwischen gestellte Uhr zuerst uebernehmen
  time_t now = nowT();
  // Abschnitt, der mit diesem Wechsel endet (fuer die Uebertragszeile, falls er ueber Mitternacht lief)
  const bool dated = g.timeValid;
  if (!dated) openNoDate();
  const bool hadSegment = dated ? g.since > kEpochSane : g.since > 0;
  const store::Carry carry{g.since, g.face, projectForFace(g.face), PROJECT_NAMES[projectForFace(g.face)]};
  if (hadSegment) {
    if (dated) accrue(g.today, projectForFace(g.face), g.since, now);
    else accrueRelative(g.noDateSec, projectForFace(g.face), g.since, now);  // ohne Uhrzeit zaehlt die Zeit seit dem Start
  }
  g.face = (int8_t)face;
  g.since = now;
  int p = projectForFace(face);
  if (!store::appendEvent(now, g.timeValid, face, p, PROJECT_NAMES[p], hal::batteryMv(), hadSegment ? &carry : nullptr))
    log_e("Log schreiben fehlgeschlagen");
  log_i("Seite %d -> %s", face, PROJECT_NAMES[p]);
  // aufsummierte Zeit des Projekts: heute (die Summen beginnen mit jedem Tag bei 0), ohne Uhrzeit die Summe ohne Datum
  const int32_t shownSec = dated ? (isSameDay(g.today, now) ? (int32_t)g.today.sec[p] : 0) : (int32_t)g.noDateSec[p];

  ui::startProjectTone(p);  // der Ton laeuft im Hintergrund, waehrend die Anzeige aufgebaut wird
  if (face == 0) {
    showStatusNow();
  } else if (displayVisible(face)) {  // sonst zeigt das Display Richtung Tisch
    ui::showProject(p, PROJECT_NAMES[p], rotationFor(face, accel), shownSec, !dated, fsPercentNow());
    uiUntil = millis() + PROJECT_SHOW_MS;
    statusShown = false;
  }
  startSync(false);
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
               "<h2>Zeitprisma</h2>");
  if (!g.timeValid) h += F("<p>Keine Uhrzeit: Summen ohne Datum seit dem Start</p>");
  h += F("<table>");
  time_t now = nowT();
  char b[16];
  uint32_t base[kFaces];
  currentTotals(base, now);
  for (int p = 1; p < kFaces; ++p) {
    uint32_t s = base[p];
    if (p == projectForFace(g.face) && g.since > 0 && now > g.since) s += (uint32_t)(now - g.since);
    formatHM(b, sizeof b, s);
    h += "<tr><td>" + String(p) + "</td><td>" + PROJECT_NAMES[p] + "</td><td align=right>" + b + "</td></tr>";
  }
  char tail[260];
  snprintf(tail, sizeof tail,
           "</table><p>Aktiv: %s<br>Akku: %.2f V<br>Logs: %u Bytes, Speicher %d %% belegt%s</p>"
           "<p><a href='/log.csv'>Alle Tage (log.csv)</a></p><p>Ein Tag:<br>",
           PROJECT_NAMES[projectForFace(g.face)], hal::batteryMv() / 1000.f, (unsigned)store::logSize(), fsPercentNow(),
           fsPercentNow() >= FS_WARN_PERCENT ? " (fast voll!)" : "");
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
  abortSync(true);  // der Export braucht die WLAN-Schnittstelle selbst
  const time_t relBefore = nowT();  // fuer den Uebergang ohne Datum -> mit Uhrzeit, falls der Export die Zeit holt
  const uint32_t ms0 = millis();
  const ClockMark m0 = clockMark();
  bool connected = false;
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
        connected = true;
      });
  if (time(nullptr) > kEpochSane && !g.timeValid) {  // Export hat nebenbei NTP geholt
    g.lastSync = time(nullptr);
    g.syncTryAt = 0;
    if (g.since > 0 || g.noDateOpen) timeFound(relBefore + (time_t)((millis() - ms0) / 1000));  // Zeit ohne Datum lief
    else g.timeValid = true;
  } else if (g.timeValid && connected) {  // der Export hat die Uhr (asynchron) gestellt: Sprung mitnehmen
    g.lastSync = nowT();
    g.syncTryAt = 0;
    followClockJump(m0);
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
    // Uhrzeit im Hintergrund holen: bis sie da ist (oder der Abgleich scheitert) zaehlt die Zeit seit dem Start
    startSync(true);
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
  pollSync();

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
  bool busy = (int32_t)(uiUntil - millis()) > 0 || (int32_t)(awakeUntil - millis()) > 0 || M5.BtnA.isPressed() ||
              syncPending;  // laeuft der Abgleich, bleibt der Stick wach (hoechstens kSyncMaxMs)
  if (!busy && tracker.settled()) goToSleep(false);
  if (!busy && millis() - sessionStart > SESSION_MAX_MS) goToSleep(true);
}

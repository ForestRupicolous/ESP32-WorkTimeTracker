#include "ui.h"

#include <M5Unified.h>

#include "config.h"

namespace ui {

void begin() {
  M5.Display.setRotation(DISP_ROT_FLAT);  // quer, 240 x 135
  M5.Display.setBrightness(110);
  M5.Speaker.setVolume(SPEAKER_VOLUME);
}

void off() { M5.Display.sleep(); }

void setRotation(uint8_t rot) { M5.Display.setRotation(rot & 3); }

static void on() {
  M5.Display.wakeup();
  M5.Display.setBrightness(110);
}

// Der Lautsprecher (ES8311-Codec plus Verstaerker) braucht nach M5.Speaker.begin() eine Anlaufzeit. Am Geraet ging der
// erste Ton nach dem Wecken verloren (Rueckmeldung 29.10.2026: kein Ton beim Wecken aus dem Liegen in ein Projekt).
// Darum frueh starten (prepareSound) und einen Ton kurz nach dem Start mit Stille beginnen lassen.
static constexpr uint32_t kWarmupMs = 200;
static uint32_t speakerStartedMs = 0;

static void ensureSpeaker() {
  if (M5.Speaker.isRunning()) return;
  M5.Speaker.begin();
  speakerStartedMs = millis();
}

static uint32_t warmupLeftMs() {
  uint32_t since = millis() - speakerStartedMs;
  return since >= kWarmupMs ? 0 : kWarmupMs - since;
}

void prepareSound() { ensureSpeaker(); }

void beep(int count, int freq, int ms) {
  ensureSpeaker();
  if (uint32_t w = warmupLeftMs()) delay(w);
  for (int i = 0; i < count; ++i) {
    M5.Speaker.tone(freq, ms);
    while (M5.Speaker.isPlaying()) delay(2);
    delay(ms);
  }
}

void longBeep() { beep(1, 1800, 350); }

// Weicher Einzelton: Sinus plus leise Oktave, kurzer Anschwung, Abklingen. M5.Speaker.tone() liefert nur Rechteck.
// Der Puffer bleibt bis zum Ende der Wiedergabe in Benutzung: waitTone() bzw. der naechste Ton wartet darauf.
static constexpr uint32_t kToneRate = 22050;  // Abtastrate des StickS3-Lautsprechers (M5Unified-Vorgabe)
static constexpr uint32_t kToneMs = 300;
static constexpr size_t kToneLen = kToneRate * kToneMs / 1000;
static constexpr size_t kLeadMax = kToneRate * kWarmupMs / 1000;  // Stille vor dem Ton, solange der Lautsprecher anlaeuft
static int16_t toneBuf[kLeadMax + kToneLen];

void waitTone() {
  while (M5.Speaker.isPlaying()) delay(2);
}

static void softTone(float hz) {
  ensureSpeaker();
  waitTone();
  size_t lead = (size_t)warmupLeftMs() * kToneRate / 1000;
  if (lead > kLeadMax) lead = kLeadMax;
  memset(toneBuf, 0, lead * sizeof(int16_t));
  const float w = 2.0f * (float)M_PI * hz / (float)kToneRate;
  for (size_t i = 0; i < kToneLen; ++i) {
    float t = (float)i / (float)kToneRate;
    float env = expf(-t / 0.11f);
    if (t < 0.008f) env *= t / 0.008f;                                // Anschwung 8 ms
    float tail = (float)(kToneLen - 1 - i) / (float)kToneRate;
    if (tail < 0.02f) env *= tail / 0.02f;                            // Ausblenden 20 ms
    float s = sinf(w * (float)i) + 0.25f * sinf(2.0f * w * (float)i) * expf(-t / 0.05f);
    toneBuf[lead + i] = (int16_t)(s * env * 23000.0f);
  }
  M5.Speaker.playRaw(toneBuf, lead + kToneLen, kToneRate, false, 1, -1, false);
}

void startProjectTone(int project) {
  const int n = (int)(sizeof(PROJECT_TONES_HZ) / sizeof(PROJECT_TONES_HZ[0]));
  if (project < 0) project = 0;
  if (project >= n) project = n - 1;
  softTone((float)PROJECT_TONES_HZ[project]);
}

void beepProject(int project) {
  startProjectTone(project);
  waitTone();
}

void showProject(int project, const char* name, uint8_t rot, int32_t todaySec) {
  setRotation(rot);
  on();
  auto& d = M5.Display;
  d.fillScreen(PROJECT_COLORS[project]);
  d.setTextColor(TFT_BLACK);
  d.setTextDatum(textdatum_t::middle_center);
  d.setFont(&fonts::Font4);
  d.drawString(name, d.width() / 2, d.height() * 3 / 10);
  // aufsummierte Zeit dieses Projekts heute, gross (ohne gueltige Uhrzeit "--:--")
  char s[16];
  if (todaySec >= 0) prisma::formatHM(s, sizeof s, (uint32_t)todaySec);
  else snprintf(s, sizeof s, "--:--");
  d.setFont(&fonts::Font6);
  d.drawString(s, d.width() / 2, d.height() * 66 / 100);
}

void showStatus(const prisma::DayTotals& tot, int currentProject, time_t since, time_t now,
                bool timeValid, int vbatMv) {
  setRotation(DISP_ROT_FLAT);
  on();
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setFont(&fonts::Font2);
  d.setTextColor(TFT_WHITE);
  d.setTextDatum(textdatum_t::top_left);
  if (!timeValid) {
    d.drawString("Zeit unbekannt", 2, 1);
  } else {
    char h[24];
    struct tm lt;
    localtime_r(&now, &lt);
    strftime(h, sizeof h, "Heute %d.%m. %H:%M", &lt);
    d.drawString(h, 2, 1);
  }
  char b[16], line[40];
  d.setTextColor(TFT_DARKGREY);
  d.setTextDatum(textdatum_t::top_right);
  snprintf(line, sizeof line, "%.2fV", vbatMv / 1000.f);
  d.drawString(line, d.width() - 2, 1);
  d.setTextDatum(textdatum_t::top_left);
  // Projekte in zwei Spalten
  const int n = prisma::kFaces - 1;
  const int rows = (n + 1) / 2;
  const int rowH = (d.height() - 20) / rows > 20 ? 20 : (d.height() - 20) / rows;
  for (int p = 1; p <= n; ++p) {
    uint32_t s = prisma::isSameDay(tot, now) ? tot.sec[p] : 0;
    if (timeValid && p == currentProject && now > since) s += (uint32_t)(now - since);
    prisma::formatHM(b, sizeof b, s);
    d.setTextColor(PROJECT_COLORS[p]);
    snprintf(line, sizeof line, "%2d %-7.7s%5s", p, PROJECT_NAMES[p], b);
    int col = (p - 1) / rows, row = (p - 1) % rows;
    d.drawString(line, 2 + col * 120, 20 + row * rowH);
  }
}

void showText(const char* l1, const char* l2, const char* l3) {
  on();
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE);
  d.setTextDatum(textdatum_t::top_left);
  d.setFont(&fonts::Font4);
  d.drawString(l1, 6, 8);
  d.setFont(&fonts::Font2);
  if (l2) d.drawString(l2, 6, 52);
  if (l3) d.drawString(l3, 6, 76);
}

}  // namespace ui

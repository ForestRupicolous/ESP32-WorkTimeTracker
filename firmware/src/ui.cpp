#include "ui.h"

#include <M5Unified.h>

#include "config.h"

namespace ui {

void begin() {
  M5.Display.setRotation(1);  // quer, 240 x 135
  M5.Display.setBrightness(110);
  M5.Speaker.setVolume(SPEAKER_VOLUME);
}

void off() { M5.Display.sleep(); }

static void on() {
  M5.Display.wakeup();
  M5.Display.setBrightness(110);
}

void beep(int count, int freq, int ms) {
  if (!M5.Speaker.isRunning()) M5.Speaker.begin();
  for (int i = 0; i < count; ++i) {
    M5.Speaker.tone(freq, ms);
    while (M5.Speaker.isPlaying()) delay(2);
    delay(ms);
  }
}

void longBeep() { beep(1, 1800, 350); }

void showProject(int project, const char* name) {
  on();
  auto& d = M5.Display;
  d.fillScreen(PROJECT_COLORS[project]);
  d.setTextColor(TFT_BLACK);
  d.setTextDatum(textdatum_t::middle_center);
  d.setFont(&fonts::Font4);
  d.drawString(name, d.width() / 2, d.height() / 2 - 8);
  d.setFont(&fonts::Font2);
  char s[16];
  snprintf(s, sizeof s, "Seite %d", project);
  d.drawString(s, d.width() / 2, d.height() - 18);
}

void showStatus(const prisma::DayTotals& tot, int currentProject, time_t since, time_t now,
                bool timeValid, int vbatMv) {
  on();
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextDatum(textdatum_t::top_left);
  d.setFont(&fonts::Font2);
  d.setTextColor(TFT_WHITE);
  if (!timeValid) {
    d.drawString("Zeit unbekannt - WLAN pruefen", 4, 4);
  } else {
    char h[24];
    struct tm lt;
    localtime_r(&now, &lt);
    strftime(h, sizeof h, "Heute %d.%m. %H:%M", &lt);
    d.drawString(h, 4, 2);
  }
  char b[16], line[40];
  int y = 22;
  for (int p = 1; p < prisma::kFaces; ++p) {
    uint32_t s = prisma::isSameDay(tot, now) ? tot.sec[p] : 0;
    if (timeValid && p == currentProject && now > since) s += (uint32_t)(now - since);
    prisma::formatHM(b, sizeof b, s);
    d.setTextColor(PROJECT_COLORS[p]);
    snprintf(line, sizeof line, "%d %-12.12s %6s", p, PROJECT_NAMES[p], b);
    d.drawString(line, 4, y);
    y += 18;
  }
  d.setTextColor(TFT_DARKGREY);
  snprintf(line, sizeof line, "Akku %.2f V   A: halten 2s WLAN", vbatMv / 1000.f);
  d.drawString(line, 4, d.height() - 16);
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

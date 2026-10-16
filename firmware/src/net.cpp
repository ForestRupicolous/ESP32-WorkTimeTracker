#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "net.h"
#include "store.h"

namespace {

bool connect() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) delay(50);
  return WiFi.status() == WL_CONNECTED;
}

void disconnect() {
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
}

}  // namespace

namespace net {

bool syncTime() {
  if (!connect()) {
    log_w("WLAN nicht erreichbar");
    disconnect();
    return false;
  }
  configTzTime(TZ_INFO, NTP_1, NTP_2);
  struct tm tm;
  bool ok = getLocalTime(&tm, 6000) && tm.tm_year > (2024 - 1900);
  disconnect();
  return ok;
}

void serveExport(const std::function<String()>& statusHtml,
                 const std::function<bool()>& keepRunning,
                 const std::function<void(const char* ip)>& onReady) {
  if (!connect()) {
    disconnect();
    onReady(nullptr);
    return;
  }
  // Zeit bei der Gelegenheit gleich mitziehen
  configTzTime(TZ_INFO, NTP_1, NTP_2);

  WebServer srv(80);
  srv.on("/", HTTP_GET, [&]() { srv.send(200, "text/html; charset=utf-8", statusHtml()); });
  srv.on("/log.csv", HTTP_GET, [&]() {
    File f = LittleFS.open(store::logPath(), FILE_READ);
    if (!f) { srv.send(404, "text/plain", "noch keine Daten"); return; }
    srv.sendHeader("Content-Disposition", "attachment; filename=zeitprisma.csv");
    srv.streamFile(f, "text/csv");
    f.close();
  });
  srv.begin();
  MDNS.begin(MDNS_NAME);
  MDNS.addService("http", "tcp", 80);
  onReady(WiFi.localIP().toString().c_str());

  uint32_t t0 = millis();
  while (millis() - t0 < WEB_TIMEOUT_MS && keepRunning()) {
    srv.handleClient();
    delay(5);
  }
  srv.stop();
  MDNS.end();
  disconnect();
}

}  // namespace net

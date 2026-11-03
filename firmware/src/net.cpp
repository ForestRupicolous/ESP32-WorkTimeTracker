#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_sntp.h>

#include <atomic>

#include "config.h"
#include "net.h"
#include "store.h"

namespace {

constexpr time_t kClockSane = 1704067200;  // 2024-01-01: aeltere Uhrzeiten gelten als nicht gestellt

// getLocalTime() kehrt bei schon gueltiger Uhr sofort zurueck, ohne dass der NTP-Server geantwortet hat. Darum meldet
// der SNTP-Dienst die erste Antwort selbst: dieses Flag wird gesetzt, sobald die Uhr gestellt wurde.
volatile bool sntpDone = false;
std::atomic<bool> clockSet{false};  // der SNTP-Dienst hat die Uhr gestellt (fuer die Hauptschleife, siehe takeClockSet())
void onSntp(struct timeval*) {
  sntpDone = true;
  clockSet = true;
}

// Zustand der Hintergrund-Task: 0 frei, 1 laeuft, 2 fertig mit Uhrzeit, 3 fertig ohne Uhrzeit
std::atomic<int> jobState{0};
std::atomic<bool> cancelJob{false};
TaskHandle_t jobTask = nullptr;

// Netze in der Reihenfolge, in der sie versucht werden (Rangfolge = Reihenfolge in config.h). Bei mehreren Netzen wird vorher
// einmal gescannt, damit nicht jedes abwesende Netz die ganze Wartezeit kostet (der WLAN-Stack meldet "Netz nicht gefunden"
// nur kurz und startet gleich einen Wiederholversuch, der Status taugt nicht zum Umschalten). Versteckte Netze fehlen im
// Scan: Sind welche in Reichweite, werden auch nicht gesehene Netze versucht. Rueckgabe: Anzahl der Versuche.
int planNets(int order[WIFI_NET_COUNT]) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  bool configured[WIFI_NET_COUNT], seen[WIFI_NET_COUNT];
  int count = 0, first = -1;
  for (int i = 0; i < WIFI_NET_COUNT; ++i) {
    configured[i] = WIFI_NETS[i].ssid && WIFI_NETS[i].ssid[0];
    seen[i] = false;
    if (configured[i]) {
      ++count;
      if (first < 0) first = i;
    }
  }
  if (count == 0) {
    log_w("kein WLAN konfiguriert");
    return 0;
  }
  bool hidden = false;
  if (count == 1) {
    seen[first] = true;  // ein einzelnes Netz wie bisher gleich versuchen, ohne Scan
  } else {
    int found = WiFi.scanNetworks(false, true, false, 120);
    if (found < 0) {  // Scan fehlgeschlagen: nichts ausschliessen
      for (int i = 0; i < WIFI_NET_COUNT; ++i) seen[i] = configured[i];
    } else {
      for (int r = 0; r < found; ++r) {
        String s = WiFi.SSID(r);
        if (s.length() == 0) {
          hidden = true;
          continue;
        }
        for (int i = 0; i < WIFI_NET_COUNT; ++i)
          if (configured[i] && s == WIFI_NETS[i].ssid) seen[i] = true;
      }
    }
    WiFi.scanDelete();
    log_i("WLAN-Scan: %d Netze, gesehen: 1=%d 2=%d 3=%d, versteckte: %d", (int)found, (int)seen[0], (int)seen[1],
          (int)seen[2], (int)hidden);
  }
  return prisma::wifiTryOrder(configured, seen, WIFI_NET_COUNT, hidden, order);
}

// Mit Netz i verbinden (hoechstens WIFI_TIMEOUT_MS), sonst die Verbindung wieder abbrechen
bool connectNet(int i) {
  WiFi.begin(WIFI_NETS[i].ssid, WIFI_NETS[i].pass);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS && !cancelJob) delay(50);
  if (WiFi.status() == WL_CONNECTED) {
    log_i("WLAN %d verbunden", i + 1);
    return true;
  }
  log_w("WLAN %d nicht verbunden", i + 1);
  WiFi.disconnect(false, false);
  return false;
}

void disconnect() {
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
}

// Der eigentliche Abgleich, blockierend; laeuft nur in der Hintergrund-Task
bool syncBlocking() {
  int order[WIFI_NET_COUNT];
  const int tries = planNets(order);
  for (int k = 0; k < tries && !cancelJob; ++k) {
    if (!connectNet(order[k])) continue;
    const bool wasValid = time(nullptr) > kClockSane;
    sntpDone = false;
    sntp_set_time_sync_notification_cb(onSntp);
    configTzTime(TZ_INFO, NTP_1, NTP_2);
    uint32_t t0 = millis();
    while (!sntpDone && millis() - t0 < 6000 && !cancelJob) delay(20);
    // Ohne gueltige Uhr zaehlt auch eine gestellte Uhr ohne Meldung (wie bisher); mit gueltiger Uhr nur die NTP-Antwort
    struct tm tm;
    if (sntpDone || (!wasValid && getLocalTime(&tm, 0) && tm.tm_year > (2024 - 1900))) {
      disconnect();
      return true;
    }
    if (cancelJob) break;
    // verbunden, aber keine Uhrzeit (z. B. Anmeldeseite im Hotspot): mit dem naechsten Netz weiter
    log_w("WLAN %d: keine Uhrzeit erhalten", order[k] + 1);
    WiFi.disconnect(false, false);
  }
  if (!cancelJob) log_w("WLAN nicht erreichbar");
  disconnect();
  return false;
}

void jobMain(void*) {
  const bool ok = syncBlocking();
  jobState = ok ? 2 : 3;
  vTaskDelete(nullptr);
}

}  // namespace

namespace net {

bool syncStart() {
  if (jobState == 1) return false;
  cancelJob = false;
  clockSet = false;
  jobState = 1;
  // Core 0 wie der WLAN-Stack; die Arduino-Schleife laeuft auf Core 1 und tastet weiter ab
  if (xTaskCreatePinnedToCore(jobMain, "wlan_sync", 8192, nullptr, 1, &jobTask, 0) != pdPASS) {
    jobState = 0;
    log_e("WLAN-Task nicht gestartet");
    return false;
  }
  return true;
}

bool syncRunning() { return jobState == 1; }

bool takeClockSet() { return clockSet.exchange(false); }

bool syncFinished(bool* ok) {
  const int s = jobState;
  if (s < 2) return false;
  if (ok) *ok = (s == 2);
  jobState = 0;
  return true;
}

void syncCancel() {
  if (jobState != 1) return;
  cancelJob = true;
  const uint32_t t0 = millis();
  while (jobState == 1 && millis() - t0 < 12000) delay(20);  // der Scan laesst sich nicht abbrechen
  if (jobState == 1) {  // haengt: letzter Ausweg
    vTaskDelete(jobTask);
    disconnect();
    jobState = 3;
    log_e("WLAN-Task abgebrochen (Zeitlimit)");
  }
}

void serveExport(const std::function<String()>& statusHtml,
                 const std::function<bool()>& keepRunning,
                 const std::function<void(const char* ip)>& onReady) {
  int order[WIFI_NET_COUNT];
  const int tries = planNets(order);
  bool connected = false;
  for (int k = 0; k < tries && !connected; ++k) connected = connectNet(order[k]);
  if (!connected) {
    disconnect();
    onReady(nullptr);
    return;
  }
  // Zeit bei der Gelegenheit gleich mitziehen
  configTzTime(TZ_INFO, NTP_1, NTP_2);

  WebServer srv(80);
  srv.on("/", HTTP_GET, [&]() { srv.send(200, "text/html; charset=utf-8", statusHtml()); });
  // Alle Logs hintereinander (eine Kopfzeile): fuer tools/auswertung.py. Mit Content-Length statt "chunked": die
  // Chunked-Antwort kam am Geraet nicht vollstaendig an (HTTP/1.1-Clients bekamen eine fehlerhafte Kodierung).
  srv.on("/log.csv", HTTP_GET, [&]() {
    srv.setContentLength(store::streamAllSize());
    srv.sendHeader("Content-Disposition", "attachment; filename=zeitprisma.csv");
    srv.send(200, "text/csv", "");
    WiFiClient c = srv.client();
    store::streamAll([&](const uint8_t* d, size_t len) { c.write(d, len); });
  });
  // Ein Tag: /tag?d=JJJJ-MM-TT (auch d=ohne_zeit und d=alt)
  srv.on("/tag", HTTP_GET, [&]() {
    String key = srv.arg("d");
    char path[40];
    if (!store::pathFromKey(key.c_str(), path, sizeof path)) { srv.send(400, "text/plain", "ungueltiger Tag"); return; }
    File f = LittleFS.open(path, FILE_READ);
    if (!f) { srv.send(404, "text/plain", "kein Log fuer diesen Tag"); return; }
    srv.sendHeader("Content-Disposition", "attachment; filename=zeitprisma_" + key + ".csv");
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

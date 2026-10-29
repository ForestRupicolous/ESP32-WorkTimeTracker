#include "store.h"

#include <LittleFS.h>
#include <Preferences.h>

#include <algorithm>
#include <vector>

namespace {
constexpr const char* kNs = "prisma";
constexpr uint32_t kModelVersion = 0x300 | prisma::kFaces;  // Version 3 + Bauform
}  // namespace

namespace store {

bool begin() {
  if (LittleFS.begin(false)) return true;
  log_w("LittleFS leer/defekt -> formatiere");
  return LittleFS.begin(true);
}

bool loadModel(prisma::FaceModel& m) {
  Preferences p;
  if (!p.begin(kNs, true)) return false;
  bool ok = p.getUInt("ver", 0) == kModelVersion &&
            p.getBytes("faces", m.up, sizeof(m.up)) == sizeof(m.up);
  p.end();
  m.valid = ok;
  return ok;
}

bool saveModel(const prisma::FaceModel& m) {
  Preferences p;
  if (!p.begin(kNs, false)) return false;
  bool ok = p.putBytes("faces", m.up, sizeof(m.up)) == sizeof(m.up);
  ok &= p.putUInt("ver", kModelVersion) > 0;
  p.end();
  return ok;
}

bool appendEvent(time_t t, bool timeValid, int face, int project, const char* name, int vbatMv,
                 const Carry* carry) {
  char path[40];
  prisma::dayLogPath(path, sizeof path, t, timeValid);
  File f = LittleFS.open(path, FILE_APPEND);  // legt die Datei an, falls es sie noch nicht gibt
  if (!f) return false;
  // neu = leer (exists() meldete am Geraet fuer eine fehlende Datei einen Fehler im Log)
  bool fresh = f.size() == 0;
  bool ok = true;
  char line[160];
  if (fresh) {
    f.print(prisma::csvHeader());
    if (timeValid && carry && carry->since > 0) {
      time_t mid = prisma::localMidnight(t);
      if (carry->since < mid) {  // der vorige Abschnitt lief ueber Mitternacht: Zustand um 00:00:00 vorneweg
        size_t n = prisma::formatCsvLine(line, sizeof line, mid, true, carry->face, carry->project, carry->name, 0);
        ok = f.write((const uint8_t*)line, n) == n;
      }
    }
  }
  size_t n = prisma::formatCsvLine(line, sizeof line, t, timeValid, face, project, name, vbatMv);
  ok = (f.write((const uint8_t*)line, n) == n) && ok;
  f.close();
  return ok;
}

namespace {
struct Entry {
  String key;
  size_t size;
};

// "log.csv" -> "alt", "log_2026-10-29.csv" -> "2026-10-29", "log_ohne_zeit.csv" -> "ohne_zeit", sonst leer
String keyFromName(const String& nameIn) {
  String n = nameIn;
  if (n.startsWith("/")) n = n.substring(1);
  if (n == "log.csv") return "alt";
  if (n.startsWith("log_") && n.endsWith(".csv")) return n.substring(4, n.length() - 4);
  return "";
}

std::vector<Entry> collect() {
  std::vector<Entry> v;
  File root = LittleFS.open("/");
  if (!root) return v;
  for (File e = root.openNextFile(); e; e = root.openNextFile()) {
    if (e.isDirectory()) continue;
    String k = keyFromName(e.name());
    if (k.length()) v.push_back({k, (size_t)e.size()});
  }
  // "alt" vor den Daten, "ohne_zeit" nach den Daten: alphabetisch ist das schon die richtige Reihenfolge nicht,
  // darum eigene Rangfolge: alt < Datum < ohne_zeit
  auto rank = [](const String& k) { return k == "alt" ? 0 : (k == "ohne_zeit" ? 2 : 1); };
  std::sort(v.begin(), v.end(), [&](const Entry& a, const Entry& b) {
    int ra = rank(a.key), rb = rank(b.key);
    return ra != rb ? ra < rb : a.key < b.key;
  });
  return v;
}

bool keyValid(const char* key) {
  if (!key) return false;
  if (!strcmp(key, "alt") || !strcmp(key, "ohne_zeit")) return true;
  if (strlen(key) != 10) return false;
  for (int i = 0; i < 10; ++i) {
    bool dash = (i == 4 || i == 7);
    if (dash ? key[i] != '-' : !isdigit((unsigned char)key[i])) return false;
  }
  return true;
}
}  // namespace

bool pathFromKey(const char* key, char* out, size_t n) {
  if (!keyValid(key)) return false;
  if (!strcmp(key, "alt")) snprintf(out, n, "/log.csv");
  else snprintf(out, n, "/log_%s.csv", key);
  return true;
}

void listLogs(const std::function<void(const char* key, size_t size)>& cb) {
  for (auto& e : collect()) cb(e.key.c_str(), e.size);
}

size_t logSize() {
  size_t sum = 0;
  for (auto& e : collect()) sum += e.size;
  return sum;
}

size_t streamAllSize() {
  size_t total = strlen(prisma::csvHeader());
  for (auto& e : collect()) {
    char path[40];
    if (!pathFromKey(e.key.c_str(), path, sizeof path)) continue;
    File f = LittleFS.open(path, FILE_READ);
    if (!f) continue;
    size_t hdr = 0;  // Kopfzeile der Datei wird nicht mitgesendet
    while (f.available()) {
      ++hdr;
      if (f.read() == '\n') break;
    }
    f.close();
    total += e.size > hdr ? e.size - hdr : 0;
  }
  return total;
}

void streamAll(const std::function<void(const uint8_t* data, size_t n)>& sink) {
  const char* hdr = prisma::csvHeader();
  sink((const uint8_t*)hdr, strlen(hdr));
  uint8_t buf[1024];
  for (auto& e : collect()) {
    char path[40];
    if (!pathFromKey(e.key.c_str(), path, sizeof path)) continue;
    File f = LittleFS.open(path, FILE_READ);
    if (!f) continue;
    // Kopfzeile der Datei ueberspringen
    while (f.available() && f.read() != '\n') {}
    while (f.available()) {
      size_t n = f.read(buf, sizeof buf);
      if (!n) break;
      sink(buf, n);
    }
    f.close();
  }
}

}  // namespace store

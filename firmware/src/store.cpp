#include "store.h"

#include <LittleFS.h>
#include <Preferences.h>

namespace {
constexpr const char* kLog = "/log.csv";
constexpr const char* kNs = "prisma";
constexpr uint32_t kModelVersion = 1;
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

bool appendEvent(time_t t, bool timeValid, int face, int project, const char* name, int vbatMv) {
  bool fresh = !LittleFS.exists(kLog);
  File f = LittleFS.open(kLog, FILE_APPEND);
  if (!f) return false;
  if (fresh) f.print(prisma::csvHeader());
  char line[160];
  size_t n = prisma::formatCsvLine(line, sizeof line, t, timeValid, face, project, name, vbatMv);
  bool ok = f.write((const uint8_t*)line, n) == n;
  f.close();
  return ok;
}

const char* logPath() { return kLog; }

size_t logSize() {
  File f = LittleFS.open(kLog, FILE_READ);
  size_t s = f ? f.size() : 0;
  if (f) f.close();
  return s;
}

}  // namespace store

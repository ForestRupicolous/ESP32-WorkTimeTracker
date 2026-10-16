#include "prisma_core.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace prisma {

static constexpr float kRad2Deg = 57.29577951f;

float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float norm(const Vec3& a) { return sqrtf(dot(a, a)); }

Vec3 normalize(const Vec3& a) {
  float n = norm(a);
  if (n < 1e-6f) return {0, 0, 0};
  return {a.x / n, a.y / n, a.z / n};
}

float angleDeg(const Vec3& a, const Vec3& b) {
  float c = dot(normalize(a), normalize(b));
  if (c > 1.f) c = 1.f;
  if (c < -1.f) c = -1.f;
  return acosf(c) * kRad2Deg;
}

FaceModel defaultModel() {
  FaceModel m{};
  for (int k = 0; k < kFaces; ++k) {
    float th = (90.f - 60.f * k) / kRad2Deg;  // Normale in XZ, Seite 0 = +Z
    m.up[k] = {cosf(th), 0.f, sinf(th)};
  }
  m.valid = false;  // nur Annahme
  return m;
}

int classify(const FaceModel& m, const Vec3& accel, float matchDeg, float* bestDeg) {
  Vec3 a = normalize(accel);
  int best = kUnknown;
  float bestC = -2.f;
  for (int k = 0; k < kFaces; ++k) {
    float c = dot(a, m.up[k]);
    if (c > bestC) { bestC = c; best = k; }
  }
  if (bestC > 1.f) bestC = 1.f;
  float deg = acosf(bestC) * kRad2Deg;
  if (bestDeg) *bestDeg = deg;
  return (deg <= matchDeg) ? best : kUnknown;
}

// ---------------------------------------------------------------- StillDetector
StillDetector::StillDetector(int windowSamples, float maxDevDeg, float gMin, float gMax)
    : n_(windowSamples > kMax ? kMax : (windowSamples < 2 ? 2 : windowSamples)),
      head_(0), count_(0), maxDev_(maxDevDeg), gMin_(gMin), gMax_(gMax) {}

void StillDetector::reset() { head_ = 0; count_ = 0; }

void StillDetector::add(const Vec3& a) {
  buf_[head_] = a;
  head_ = (head_ + 1) % n_;
  if (count_ < n_) ++count_;
}

Vec3 StillDetector::mean() const {
  Vec3 s{0, 0, 0};
  for (int i = 0; i < count_; ++i) { s.x += buf_[i].x; s.y += buf_[i].y; s.z += buf_[i].z; }
  return normalize(s);
}

bool StillDetector::still() const {
  if (count_ < n_) return false;
  Vec3 m = mean();
  float cosMax = cosf(maxDev_ / kRad2Deg);
  for (int i = 0; i < count_; ++i) {
    float g = norm(buf_[i]);
    if (g < gMin_ || g > gMax_) return false;
    if (dot(normalize(buf_[i]), m) < cosMax) return false;
  }
  return true;
}

// ---------------------------------------------------------------- Tracker
Tracker::Tracker(uint32_t settleMs)
    : settleMs_(settleMs), committed_(kUnknown), candidate_(kUnknown), hasCandidate_(false), since_(0) {}

void Tracker::restore(int committedFace) {
  committed_ = committedFace;
  candidate_ = committedFace;
  hasCandidate_ = false;
}

bool Tracker::update(int face, bool still, uint32_t nowMs) {
  if (!still) {            // in Bewegung: nichts uebernehmen, Zeit neu starten
    hasCandidate_ = false;
    return false;
  }
  if (!hasCandidate_ || face != candidate_) {
    candidate_ = face;
    since_ = nowMs;
    hasCandidate_ = true;
    return false;
  }
  if (candidate_ != committed_ && (uint32_t)(nowMs - since_) >= settleMs_) {
    committed_ = candidate_;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------- TeachIn
bool TeachIn::offer(const Vec3& stillMean) {
  if (done()) return false;
  Vec3 a = normalize(stillMean);
  for (int i = 0; i < n_; ++i)
    if (angleDeg(a, v_[i]) < minSep_) return false;
  // Ab zwei Seiten ist die Rollachse bekannt: Lagen weit ausserhalb der Rollebene
  // (z. B. aufrecht hingestellt) sind keine Seite.
  if (n_ >= 2) {
    const Vec3& p = v_[0];
    const Vec3& q = v_[1];
    Vec3 axis = normalize(Vec3{p.y * q.z - p.z * q.y, p.z * q.x - p.x * q.z, p.x * q.y - p.y * q.x});
    if (fabsf(dot(a, axis)) > 0.5f) return false;  // > 30 Grad aus der Ebene
  }
  v_[n_++] = a;
  return true;
}

bool TeachIn::plausible(float tolDeg, float* worstDeg) const {
  if (!done()) return false;
  float worst = 0.f;
  for (int k = 0; k < kFaces; ++k) {
    float d = fabsf(angleDeg(v_[k], v_[(k + 1) % kFaces]) - 60.f);
    if (d > worst) worst = d;
  }
  if (worstDeg) *worstDeg = worst;
  return worst <= tolDeg;
}

FaceModel TeachIn::result() const {
  FaceModel m = defaultModel();
  for (int k = 0; k < n_ && k < kFaces; ++k) m.up[k] = v_[k];
  m.valid = done();
  return m;
}

// ---------------------------------------------------------------- Tagessummen
void clearTotals(DayTotals& d) { memset(&d, 0, sizeof(d)); d.year = -1; d.yday = -1; }

bool isSameDay(const DayTotals& d, time_t t) {
  struct tm lt;
  localtime_r(&t, &lt);
  return d.year == lt.tm_year && d.yday == lt.tm_yday;
}

void accrue(DayTotals& d, int project, time_t from, time_t to) {
  if (project < 0 || project >= kFaces || to <= from) return;
  while (from < to) {
    struct tm lt;
    localtime_r(&from, &lt);
    struct tm nm = lt;
    nm.tm_mday += 1;
    nm.tm_hour = nm.tm_min = nm.tm_sec = 0;
    nm.tm_isdst = -1;
    time_t nextMidnight = mktime(&nm);
    time_t segEnd = (to < nextMidnight) ? to : nextMidnight;
    if (d.year != lt.tm_year || d.yday != lt.tm_yday) {
      clearTotals(d);
      d.year = (int16_t)lt.tm_year;
      d.yday = (int16_t)lt.tm_yday;
    }
    d.sec[project] += (uint32_t)(segEnd - from);
    from = segEnd;
  }
}

void formatHM(char* buf, size_t n, uint32_t seconds) {
  uint32_t m = (seconds + 30) / 60;
  snprintf(buf, n, "%lu:%02lu", (unsigned long)(m / 60), (unsigned long)(m % 60));
}

const char* csvHeader() { return "zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok\n"; }

size_t formatCsvLine(char* buf, size_t n, time_t t, bool timeValid, int face, int project,
                     const char* name, int vbatMv) {
  char iso[32] = "";
  if (timeValid) {
    struct tm lt;
    localtime_r(&t, &lt);
    strftime(iso, sizeof(iso), "%Y-%m-%dT%H:%M:%S%z", &lt);
  }
  // Komma/Anfuehrungszeichen im Namen vermeiden (einfaches CSV)
  char safe[40];
  size_t j = 0;
  for (size_t i = 0; name && name[i] && j + 1 < sizeof(safe); ++i)
    safe[j++] = (name[i] == ',' || name[i] == '"' || name[i] == '\n') ? ' ' : name[i];
  safe[j] = 0;
  int r = snprintf(buf, n, "%s,%lld,%d,%d,%s,%d,%d\n", iso, (long long)t, face, project, safe, vbatMv,
                   timeValid ? 1 : 0);
  return (r < 0) ? 0 : (size_t)r;
}

}  // namespace prisma

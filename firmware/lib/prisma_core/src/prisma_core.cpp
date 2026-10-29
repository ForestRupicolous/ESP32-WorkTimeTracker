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
  if (kCoin) {
    m.up[0] = {0.f, 0.f, 1.f};
    for (int k = 1; k < kFaces; ++k) {
      float th = (90.f - kStepDeg * k) / kRad2Deg;
      m.up[k] = {cosf(th), sinf(th), 0.f};
    }
  } else {
    for (int k = 0; k < kFaces; ++k) {
      float th = (90.f - kStepDeg * k) / kRad2Deg;  // Normale in XZ, Seite 0 = +Z
      m.up[k] = {cosf(th), 0.f, sinf(th)};
    }
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

static Vec3 cross3(const Vec3& a, const Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Drehachse des Rings (Einheitsvektor). Muenze: Display-Normale (Seite 0). Prisma: Normale der
// Ringebene aus den Kreuzprodukten benachbarter Ringseiten (das Vorzeichen ist egal, es zaehlt der Betrag).
static Vec3 ringAxis(const FaceModel& m) {
  if (kCoin) return normalize(m.up[0]);
  Vec3 s{0, 0, 0};
  for (int i = 0; i < kRing; ++i) {
    Vec3 c = cross3(m.up[kRingFirst + i], m.up[kRingFirst + (i + 1) % kRing]);
    s.x += c.x; s.y += c.y; s.z += c.z;
  }
  return normalize(s);
}

int classifyRing(const FaceModel& m, const Vec3& accel, int current, const RingParams& p) {
  Vec3 a = normalize(accel);
  if (norm(a) < 0.5f) return kUnknown;  // kein brauchbarer Messwert
  Vec3 ax = ringAxis(m);
  float s = dot(a, ax);
  if (s > 1.f) s = 1.f;
  if (s < -1.f) s = -1.f;
  float phi = acosf(s) * kRad2Deg;  // Winkel zur Drehachse (0 = Display oben)
  float tilt = fabsf(90.f - phi);   // Abstand von der Ringebene
  bool inRing = current >= kRingFirst && current < kRingFirst + kRing;
  if (tilt > (inRing ? p.uprightExitDeg : p.uprightDeg)) {
    // nicht "in der Drehachse": Muenze auf dem Ruecken = Seite 0, sonst Pause (unbekannt)
    if (kCoin && phi <= (current == 0 ? p.flatExitDeg : p.flatDeg)) return 0;
    return kUnknown;
  }
  // naechste Ringseite nach dem Winkel in der Ringebene (Messwert und Seitenvektoren dorthin projiziert)
  Vec3 pr{a.x - s * ax.x, a.y - s * ax.y, a.z - s * ax.z};
  float pn = norm(pr);
  if (pn < 1e-3f) return kUnknown;
  int best = kUnknown;
  float bestAng = 1e9f, curAng = 1e9f;
  for (int i = 0; i < kRing; ++i) {
    int k = kRingFirst + i;
    float d = dot(m.up[k], ax);
    Vec3 f{m.up[k].x - d * ax.x, m.up[k].y - d * ax.y, m.up[k].z - d * ax.z};
    float fn = norm(f);
    if (fn < 1e-3f) continue;
    float c = dot(pr, f) / (pn * fn);
    if (c > 1.f) c = 1.f;
    if (c < -1.f) c = -1.f;
    float ang = acosf(c) * kRad2Deg;
    if (ang < bestAng) { bestAng = ang; best = k; }
    if (k == current) curAng = ang;
  }
  // Hysterese: bisherige Seite behalten, solange keine andere um mehr als hystDeg naeher liegt
  if (inRing && best != current && curAng < 1e8f && curAng - bestAng <= p.hystDeg) return current;
  return best;
}

bool displayVisible(int face) {
  if (face < 0) return false;
  if (kCoin) return true;  // Ruecken unten: Display oben; aufgestellt: Display senkrecht
  // Prisma: Display-Normale steht face*kStepDeg von "oben" weg; sichtbar bis knapp ueber waagrecht
  return cosf(face * kStepDeg / kRad2Deg) > 0.2f;
}

int upQuadrant(const Vec3& a) {
  float ang = atan2f(a.x, a.y) * kRad2Deg;  // 0 = +Y, 90 = +X
  int q = (int)lroundf(ang / 90.f);
  return ((q % 4) + 4) % 4;
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

// ---------------------------------------------------------------- ShakeDetector
ShakeDetector::ShakeDetector(float jerkG, int peaks, uint32_t windowMs, uint32_t minGapMs, uint32_t maxSampleGapMs)
    : jerkG_(jerkG),
      peaks_(peaks < 1 ? 1 : (peaks > kMaxPeaks ? kMaxPeaks : peaks)),
      windowMs_(windowMs),
      minGapMs_(minGapMs),
      maxGapMs_(maxSampleGapMs),
      prev_{0, 0, 0},
      prevMs_(0),
      hasPrev_(false),
      d1_(0),
      d2_(0),
      t1_(0),
      nd_(0),
      n_(0) {}

void ShakeDetector::reset() {
  hasPrev_ = false;
  nd_ = 0;
  n_ = 0;
}

bool ShakeDetector::addPeak(uint32_t tMs) {
  if (n_ > 0 && (uint32_t)(tMs - peakMs_[n_ - 1]) < minGapMs_) return false;  // Nachschwinger desselben Stosses
  if (n_ == peaks_) {
    for (int i = 1; i < n_; ++i) peakMs_[i - 1] = peakMs_[i];
    --n_;
  }
  peakMs_[n_++] = tMs;
  if (n_ == peaks_ && (uint32_t)(tMs - peakMs_[0]) <= windowMs_) {
    n_ = 0;
    return true;
  }
  return false;
}

bool ShakeDetector::add(const Vec3& a, uint32_t nowMs) {
  bool hit = false;
  if (hasPrev_ && (uint32_t)(nowMs - prevMs_) <= maxGapMs_) {
    float dx = a.x - prev_.x, dy = a.y - prev_.y, dz = a.z - prev_.z;
    float d = sqrtf(dx * dx + dy * dy + dz * dz);
    // d1_ ist eine Spitze, wenn sie ueber der Schwelle liegt und weder der Vorgaenger noch der Nachfolger groesser ist
    if (nd_ >= 1 && d1_ > jerkG_ && (nd_ < 2 || d1_ >= d2_) && d1_ > d) hit = addPeak(t1_);
    d2_ = d1_;
    d1_ = d;
    t1_ = nowMs;
    if (nd_ < 2) ++nd_;
  } else {
    nd_ = 0;  // Luecke: Aenderungsfolge neu beginnen
  }
  prev_ = a;
  prevMs_ = nowMs;
  hasPrev_ = true;
  return hit;
}

// ---------------------------------------------------------------- TeachIn
bool TeachIn::offer(const Vec3& stillMean) {
  if (done()) return false;
  Vec3 a = normalize(stillMean);
  for (int i = 0; i < n_; ++i)
    if (angleDeg(a, v_[i]) < minSep_) return false;
  // Reihenfolge: jede weitere Ringseite liegt ~kStepDeg neben der zuletzt erfassten
  if (n_ > kRingFirst && fabsf(angleDeg(a, v_[n_ - 1]) - kStepDeg) > 0.4f * kStepDeg) return false;
  // Lagen weit ausserhalb der Ringebene (Prisma aufrecht, Muenze Display unten) sind keine Seite
  if (kCoin) {
    if (n_ >= 1 && fabsf(dot(a, v_[0])) > 0.5f) return false;  // > 30 Grad aus der Randebene
  } else if (n_ >= 2) {
    const Vec3& p = v_[0];
    const Vec3& q = v_[1];
    Vec3 axis = normalize(Vec3{p.y * q.z - p.z * q.y, p.z * q.x - p.x * q.z, p.x * q.y - p.y * q.x});
    if (fabsf(dot(a, axis)) > 0.5f) return false;
  }
  v_[n_++] = a;
  return true;
}

bool TeachIn::plausible(float tolDeg, float* worstDeg) const {
  if (!done()) return false;
  float worst = 0.f;
  for (int i = 0; i < kRing; ++i) {
    const Vec3& a = v_[kRingFirst + i];
    const Vec3& b = v_[kRingFirst + (i + 1) % kRing];
    float d = fabsf(angleDeg(a, b) - kStepDeg);
    if (d > worst) worst = d;
  }
  if (kCoin) {
    for (int k = 1; k < kFaces; ++k) {
      float d = fabsf(angleDeg(v_[0], v_[k]) - 90.f);
      if (d > worst) worst = d;
    }
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

void dayLogPath(char* buf, size_t n, time_t t, bool timeValid) {
  if (!timeValid) {
    snprintf(buf, n, "/log_ohne_zeit.csv");
    return;
  }
  struct tm lt;
  localtime_r(&t, &lt);
  snprintf(buf, n, "/log_%04d-%02d-%02d.csv", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday);
}

time_t localMidnight(time_t t) {
  struct tm lt;
  localtime_r(&t, &lt);
  lt.tm_hour = lt.tm_min = lt.tm_sec = 0;
  lt.tm_isdst = -1;
  return mktime(&lt);
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

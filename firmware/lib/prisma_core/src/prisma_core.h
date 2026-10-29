// Zeitprisma – hardwareunabhaengige Logik (auf dem PC testbar, siehe test/host)
//  - Seitenerkennung aus dem Beschleunigungsvektor
//  - Ruheerkennung und Entprellen (Seite muss SETTLE_MS stabil liegen)
//  - Teach-in der Seitenvektoren
//
// Bauform per Build-Flag:
//   (Standard)          Muenze mit PRISMA_RING Randseiten (Standard 8): Seite 0 = Ruecken unten
//                       (Display oben) = Pause, Randseiten 1..N, aktiv ist die OBEN liegende
//   -DPRISMA_RING=12    Muenze mit 12 Randseiten
//   -DPRISMA_PRISMA10   Zehnkant-Prisma: Seite 0 (Display) = Pause, 1..9 = Projekte
//  - Tagessummen mit Mitternachts-Split
//  - CSV-Zeile
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>

namespace prisma {

#if defined(PRISMA_PRISMA10)
constexpr bool kCoin   = false;
constexpr int  kRing   = 10;          // Seiten im Ring (inkl. Seite 0)
constexpr int  kFaces  = 10;          // Seite 0 = Display, 1..9 = Projekte
constexpr int  kRingFirst = 0;        // Seite 0 liegt im Ring
#else
#ifndef PRISMA_RING
#define PRISMA_RING 8
#endif
constexpr bool kCoin   = true;
constexpr int  kRing   = PRISMA_RING; // Randseiten 1..N
constexpr int  kFaces  = kRing + 1;   // + Seite 0 = Ruecken unten (Display oben)
constexpr int  kRingFirst = 1;
static_assert(kRing >= 6 && kRing <= 12, "PRISMA_RING 6..12");
#endif
constexpr float kStepDeg = 360.0f / kRing;  // Winkel zwischen Nachbarseiten im Ring
constexpr int kUnknown = -1;  // keine Seite erkannt (Prisma aufrecht, Muenze Display unten) -> Pause

struct Vec3 { float x, y, z; };

float dot(const Vec3& a, const Vec3& b);
float norm(const Vec3& a);
Vec3  normalize(const Vec3& a);
float angleDeg(const Vec3& a, const Vec3& b);

// Beschleunigungsvektor (normiert), wenn Seite k OBEN liegt.
struct FaceModel {
  Vec3 up[kFaces];
  bool valid;
};

// Annahme ohne Teach-in (nur Notloesung, Teach-in ist Pflicht):
//  Prisma: Laengsachse = +Y, Display (Seite 0) = +Z, Seiten im Uhrzeigersinn um +Y
//  Muenze: Seite 0 = +Z, Randseite k oben = Richtung 90-kStepDeg*k Grad in der XY-Ebene
FaceModel defaultModel();

// Seite, deren Vektor maximal matchDeg vom Messwert abweicht, sonst kUnknown.
int classify(const FaceModel& m, const Vec3& accel, float matchDeg, float* bestDeg = nullptr);

// Seite 0 (Display oben) und unbekannt -> Projekt 0 = Pause, Seite k -> Projekt k
inline int projectForFace(int face) { return (face <= 0) ? 0 : face; }

// Zeigt das Display nach oben/seitlich (lohnt sich eine Anzeige), wenn 'face' oben liegt?
bool displayVisible(int face);

// Muenze aufgestellt: in welche Richtung der Displayebene zeigt "oben"?
// 0 = +Y, 1 = +X, 2 = -Y, 3 = -X (Sensorachsen wie M5Unified liefert), naechster 90-Grad-Schritt.
int upQuadrant(const Vec3& accel);

// ---------------------------------------------------------------- Ruhe
// Gleitendes Fenster: "ruhig", wenn alle Samples im Fenster weniger als maxDevDeg
// vom Mittel abweichen und der Betrag plausibel ~1 g ist.
class StillDetector {
 public:
  static constexpr int kMax = 64;
  StillDetector(int windowSamples, float maxDevDeg, float gMin = 0.80f, float gMax = 1.20f);
  void reset();
  void add(const Vec3& a);
  bool still() const;
  Vec3 mean() const;  // normiert
 private:
  Vec3 buf_[kMax];
  int n_, head_, count_;
  float maxDev_, gMin_, gMax_;
};

// ---------------------------------------------------------------- Entprellen
class Tracker {
 public:
  explicit Tracker(uint32_t settleMs);
  void restore(int committedFace);
  // still=false setzt die Kandidaten-Zeit zurueck. Liefert true, wenn ein neuer
  // Zustand uebernommen wurde (committed() ist dann der neue Wert).
  bool update(int face, bool still, uint32_t nowMs);
  int committed() const { return committed_; }
  int candidate() const { return candidate_; }
  bool settled() const { return candidate_ == committed_ && hasCandidate_; }
 private:
  uint32_t settleMs_;
  int committed_, candidate_;
  bool hasCandidate_;
  uint32_t since_;
};

// ---------------------------------------------------------------- Teach-in
// Seite 0 oben starten, dann der Reihe nach die Seiten 1.. nach oben bringen. Eine neue
// ruhige Lage wird die naechste Seite, wenn sie >= minSepDeg von allen bisherigen entfernt
// ist, in der Ringebene liegt und ~kStepDeg (+-40 %) neben der zuletzt erfassten Ringseite.
class TeachIn {
 public:
  explicit TeachIn(float minSepDeg = 0.65f * kStepDeg) : minSep_(minSepDeg), n_(0) {}
  void start() { n_ = 0; }
  // true, wenn die Lage als neue Seite uebernommen wurde
  bool offer(const Vec3& stillMean);
  int captured() const { return n_; }
  bool done() const { return n_ >= kFaces; }
  // Plausibilitaet: Nachbarseiten im Ring ~kStepDeg auseinander (Toleranz tolDeg);
  // Muenze zusaetzlich: Randseiten ~90 Grad zu Seite 0
  bool plausible(float tolDeg = 8.0f, float* worstDeg = nullptr) const;
  FaceModel result() const;
 private:
  float minSep_;
  int n_;
  Vec3 v_[kFaces];
};

// ---------------------------------------------------------------- Tagessummen
struct DayTotals {
  int16_t year;   // tm_year
  int16_t yday;   // tm_yday
  uint32_t sec[kFaces];  // je Projekt 0..kFaces-1
};

void clearTotals(DayTotals& d);
// Zeit [from, to) dem Projekt gutschreiben; teilt an lokaler Mitternacht und
// behaelt nur den Tag von 'to' (aeltere Tage werden verworfen).
void accrue(DayTotals& d, int project, time_t from, time_t to);
bool isSameDay(const DayTotals& d, time_t t);

// "1:05" (h:mm)
void formatHM(char* buf, size_t n, uint32_t seconds);

// CSV: zeit,epoch,seite,projekt,name,vbat_mv,zeit_ok
const char* csvHeader();
size_t formatCsvLine(char* buf, size_t n, time_t t, bool timeValid, int face, int project,
                     const char* name, int vbatMv);

}  // namespace prisma

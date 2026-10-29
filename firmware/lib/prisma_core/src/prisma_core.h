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
// Nur noch fuer Tests/Hilfszwecke; im Betrieb erkennt classifyRing() (keine Totzone).
int classify(const FaceModel& m, const Vec3& accel, float matchDeg, float* bestDeg = nullptr);

// ---------------------------------------------------------------- Ringerkennung ohne Totzone
// Betriebserkennung. Steht der Stick "in der Drehachse" (Rollachse ca. waagrecht, der
// Beschleunigungsvektor liegt ca. in der Ringebene), wird IMMER eine Ringseite geliefert, auch
// zwischen zwei Seiten. Ringebene = Ebene senkrecht zur Drehachse (Muenze: Display-Normale,
// Prisma: Laengsachse). Hysterese an drei Stellen, jeweils bezogen auf die bisherige Seite 'current':
//  - Nachbarseiten: von 'current' wird erst gewechselt, wenn eine andere Ringseite um mehr als
//    hystDeg naeher am Messwert liegt (Winkel in der Ringebene).
//  - Ringebene: Eintritt bei <= uprightDeg Abstand zur Ebene, Verbleib bis uprightExitDeg.
//  - Muenze, Seite 0 (Display oben): Eintritt bei <= flatDeg von Seite 0, Verbleib bis flatExitDeg.
// Sonst kUnknown (Muenze: Display unten oder schraeg; Prisma: aufrecht stehend) = Pause.
struct RingParams {
  float hystDeg;         // Hysterese zwischen Nachbarseiten
  float uprightDeg;      // Eintritt in die Ringebene: hoechstens so weit aus der Ebene gekippt
  float uprightExitDeg;  // Verbleib in der Ringebene: hoechstens so weit aus der Ebene gekippt
  float flatDeg;         // Muenze: Seite 0, wenn hoechstens so weit von "Display oben" entfernt
  float flatExitDeg;     // Muenze: Seite 0 behalten bis zu dieser Entfernung
};
constexpr float kRingHystFrac = 0.20f;  // hystDeg = kRingHystFrac * kStepDeg (8 Seiten: 9 Grad)
constexpr float kUprightDeg = 35.0f;
constexpr float kUprightExitDeg = 45.0f;
constexpr float kFlatDeg = 30.0f;
constexpr float kFlatExitDeg = 40.0f;
inline RingParams defaultRingParams() {
  return {kRingHystFrac * kStepDeg, kUprightDeg, kUprightExitDeg, kFlatDeg, kFlatExitDeg};
}
int classifyRing(const FaceModel& m, const Vec3& accel, int current, const RingParams& p);

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

// ---------------------------------------------------------------- Schuetteln
// Erkennt Schuetteln aus dem Beschleunigungsvektor: mehrere kraeftige Spitzen kurz hintereinander. Eine Spitze ist ein
// lokales Maximum der Aenderung |a(k) - a(k-1)| (in g) ueber jerkG. Ein einzelner Stoss (Aufsetzen, Anstossen) liefert
// ein bis zwei Spitzen dicht beieinander, Schuetteln eine je Halbperiode; Spitzen naeher als minGapMs zaehlen als
// eine. Samples mit mehr als maxSampleGapMs Abstand (z. B. nach einem Ton) werden nicht miteinander verglichen.
// Langsames Drehen (Rollen, Umlegen) bleibt weit unter der Schwelle.
class ShakeDetector {
 public:
  static constexpr int kMaxPeaks = 8;
  ShakeDetector(float jerkG, int peaks, uint32_t windowMs, uint32_t minGapMs = 70, uint32_t maxSampleGapMs = 60);
  void reset();
  // true, sobald 'peaks' Spitzen innerhalb von windowMs lagen; danach beginnt die Zaehlung neu
  bool add(const Vec3& a, uint32_t nowMs);
 private:
  bool addPeak(uint32_t tMs);
  float jerkG_;
  int peaks_;
  uint32_t windowMs_, minGapMs_, maxGapMs_;
  Vec3 prev_;
  uint32_t prevMs_;
  bool hasPrev_;
  float d1_, d2_;   // letzte beiden Aenderungen (d1_ = juengste)
  uint32_t t1_;     // Zeitpunkt von d1_
  int nd_;          // gueltige Aenderungen in d1_/d2_ (0..2)
  uint32_t peakMs_[kMaxPeaks];
  int n_;           // gezaehlte Spitzen im Fenster
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

// Tageslog: "/log_JJJJ-MM-TT.csv" (lokales Datum von t), ohne gueltige Uhrzeit "/log_ohne_zeit.csv"
void dayLogPath(char* buf, size_t n, time_t t, bool timeValid);
// lokale Mitternacht (00:00:00) des Tages von t
time_t localMidnight(time_t t);

}  // namespace prisma

// Zeitprisma – hardwareunabhaengige Logik (auf dem PC testbar, siehe test/host)
//  - Seitenerkennung aus dem Beschleunigungsvektor
//  - Ruheerkennung und Entprellen (Seite muss SETTLE_MS stabil liegen)
//  - Teach-in der sechs Seitenvektoren
//  - Tagessummen mit Mitternachts-Split
//  - CSV-Zeile
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>

namespace prisma {

constexpr int kFaces   = 6;   // Seite 0 = Display, 1..5 = Projekte
constexpr int kUnknown = -1;  // keine Seite oben (z. B. aufrecht stehend) -> Pause

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

// Annahme ohne Teach-in: Laengsachse = +Y, Display (Seite 0) = +Z,
// Seiten im Uhrzeigersinn um +Y. Nur Notloesung – Teach-in ist Pflicht.
FaceModel defaultModel();

// Seite, deren Vektor maximal matchDeg vom Messwert abweicht, sonst kUnknown.
int classify(const FaceModel& m, const Vec3& accel, float matchDeg, float* bestDeg = nullptr);

// Seite 0 (Display oben) und unbekannt -> Projekt 0 = Pause, Seite k -> Projekt k
inline int projectForFace(int face) { return (face <= 0) ? 0 : face; }

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
// Seite 0 oben starten, dann der Reihe nach auf 1..5 rollen. Jede neue ruhige
// Lage, die >= minSepDeg von allen bisherigen entfernt ist, wird die naechste Seite.
class TeachIn {
 public:
  explicit TeachIn(float minSepDeg = 40.0f) : minSep_(minSepDeg), n_(0) {}
  void start() { n_ = 0; }
  // true, wenn die Lage als neue Seite uebernommen wurde
  bool offer(const Vec3& stillMean);
  int captured() const { return n_; }
  bool done() const { return n_ >= kFaces; }
  // Plausibilitaet: Nachbarseiten ~60 Grad auseinander (Toleranz tolDeg)
  bool plausible(float tolDeg = 15.0f, float* worstDeg = nullptr) const;
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
  uint32_t sec[kFaces];  // je Projekt 0..5
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

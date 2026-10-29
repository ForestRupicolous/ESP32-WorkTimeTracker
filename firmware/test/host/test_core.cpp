// Host-Tests fuer prisma_core:  make -C test/host
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random>

#include "prisma_core.h"

using namespace prisma;

static int g_fail = 0, g_pass = 0;
#define CHECK(c)                                                              \
  do {                                                                        \
    if (c) ++g_pass;                                                          \
    else { ++g_fail; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); }    \
  } while (0)

// Rotation um beliebige Achse (Rodrigues)
static Vec3 rot(Vec3 v, Vec3 k, float deg) {
  k = normalize(k);
  float t = deg * 3.14159265f / 180.f, c = cosf(t), s = sinf(t);
  Vec3 kxv{k.y * v.z - k.z * v.y, k.z * v.x - k.x * v.z, k.x * v.y - k.y * v.x};
  float kd = dot(k, v);
  return {v.x * c + kxv.x * s + k.x * kd * (1 - c), v.y * c + kxv.y * s + k.y * kd * (1 - c),
          v.z * c + kxv.z * s + k.z * kd * (1 - c)};
}

// Simuliertes Geraet mit schiefer, unbekannter Sensorlage.
//  Prisma: axis = Rollachse, ref = Beschleunigung bei Seite 0 oben
//  Muenze: axis = Display-Normale (Seite 0 oben), ref = Randseite 1 oben
struct Sim {
  Vec3 axis, ref;
  Vec3 face(int k, float err = 0) const {
    if (kCoin) {
      if (k == 0) return rot(axis, ref, err);
      return rot(ref, axis, -kStepDeg * (k - 1) + err);
    }
    return rot(ref, axis, -kStepDeg * k + err);
  }
  // Lage, die keine Seite ist: Prisma aufrecht, Muenze mit Display nach unten
  Vec3 offFace() const { return kCoin ? Vec3{-axis.x, -axis.y, -axis.z} : axis; }
};

static Sim makeSim() {
  Sim s{normalize(Vec3{0.3f, 0.9f, -0.2f}), {0, 0, 0}};
  Vec3 tmp{0, 0, 1};
  float d = dot(tmp, s.axis);
  s.ref = normalize(Vec3{tmp.x - d * s.axis.x, tmp.y - d * s.axis.y, tmp.z - d * s.axis.z});
  return s;
}

static constexpr float kMatch = 0.4f * kStepDeg;  // wie MATCH_DEG in config.h

static void test_layout() {
#if defined(PRISMA_PRISMA10)
  CHECK(!kCoin && kFaces == 10 && kRing == 10 && fabsf(kStepDeg - 36.f) < 1e-4f);
#else
  CHECK(kCoin && kFaces == PRISMA_RING + 1 && kRing == PRISMA_RING);
  CHECK(fabsf(kStepDeg * kRing - 360.f) < 1e-3f);
#endif
  printf("  Bauform: %s, %d Seiten, Fangbereich +-%.1f Grad\n", kCoin ? "Muenze" : "Prisma", kFaces, kMatch);
  CHECK(kMatch < kStepDeg / 2);  // Fangbereiche ueberlappen nicht
}

static void test_classify_default() {
  FaceModel m = defaultModel();
  for (int k = 0; k < kFaces; ++k) CHECK(classify(m, m.up[k], kMatch) == k);
  for (int k = 0; k < kFaces; ++k)
    for (int j = k + 1; j < kFaces; ++j) CHECK(angleDeg(m.up[k], m.up[j]) > 2 * kMatch);
  // im Ring: 10 Grad daneben -> erkannt, genau zwischen zwei Seiten -> unbekannt
  Vec3 ax = kCoin ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
  CHECK(classify(m, rot(m.up[2], ax, 10.f), kMatch) == 2);
  CHECK(classify(m, rot(m.up[2], ax, kStepDeg / 2), kMatch) == kUnknown);
  CHECK(classify(m, rot(m.up[2], ax, kMatch + 2), kMatch) == kUnknown);
  if (kCoin) {
    CHECK(classify(m, Vec3{0, 0, -1}, kMatch) == kUnknown);        // Display unten
    CHECK(classify(m, normalize(Vec3{0, 1, 1}), kMatch) == kUnknown); // halb gekippt
  } else {
    CHECK(classify(m, Vec3{0, 1, 0}, kMatch) == kUnknown);         // aufrecht
    CHECK(classify(m, Vec3{0, -1, 0}, kMatch) == kUnknown);
  }
  CHECK(projectForFace(kUnknown) == 0 && projectForFace(0) == 0 && projectForFace(kFaces - 1) == kFaces - 1);
}

static void test_display_visible() {
  int vis = 0;
  for (int k = 0; k < kFaces; ++k) vis |= displayVisible(k) << k;
#if defined(PRISMA_PRISMA10)
  CHECK(vis == 0b1100000111);
#else
  CHECK(vis == (1 << kFaces) - 1);
#endif
  CHECK(!displayVisible(kUnknown));
}

static void test_up_quadrant() {
  CHECK(upQuadrant(Vec3{0, 1, 0}) == 0);
  CHECK(upQuadrant(Vec3{1, 0, 0}) == 1);
  CHECK(upQuadrant(Vec3{0, -1, 0}) == 2);
  CHECK(upQuadrant(Vec3{-1, 0, 0}) == 3);
  CHECK(upQuadrant(Vec3{0.5f, 0.866f, 0}) == 0);   // 30 Grad
  CHECK(upQuadrant(Vec3{0.866f, 0.5f, 0}) == 1);   // 60 Grad
  CHECK(upQuadrant(Vec3{-0.866f, -0.5f, 0}) == 3); // 240 Grad
  CHECK(upQuadrant(Vec3{-0.5f, -0.866f, 0}) == 2); // 210 Grad
}

static void test_teachin_and_classify() {
  Sim s = makeSim();
  TeachIn t;
  t.start();
  CHECK(t.offer(s.face(0, 1.5f)));
  CHECK(!t.offer(s.face(0, -2.f)));          // gleiche Seite nochmal -> ignoriert
  CHECK(!t.offer(s.offFace()));              // keine Seite
  for (int k = 1; k < kFaces; ++k) {
    if (k >= 2) CHECK(!t.offer(s.face(k - 1, kStepDeg * 0.4f)));  // beim Rollen haengen geblieben
    if (k >= 2) CHECK(!t.offer(s.offFace()));
    CHECK(t.offer(s.face(k, (k % 2) ? 2.f : -2.5f)));
  }
  CHECK(t.done());
  CHECK(!t.offer(s.face(1, 0.f)));
  float worst = 0;
  CHECK(t.plausible(8.f, &worst));
  FaceModel m = t.result();
  CHECK(m.valid);

  std::mt19937 rng(42);
  std::normal_distribution<float> noise(0.f, 0.02f);
  int ok = 0, tot = 0;
  for (int k = 0; k < kFaces; ++k)
    for (int i = 0; i < 200; ++i) {
      Vec3 a = s.face(k, noise(rng) * 100.f);  // 2 Grad Lagefehler (1 sigma): Tisch, Auflage
      a.x += noise(rng); a.y += noise(rng); a.z += noise(rng);  // 20 mg Sensorrauschen
      ok += classify(m, a, kMatch) == k;
      ++tot;
    }
  printf("  Klassifikation mit Rauschen: %d/%d korrekt (Teach-in Abweichung max %.1f Grad)\n", ok, tot, worst);
  CHECK(ok >= tot * 995 / 1000);
  CHECK(classify(m, s.offFace(), kMatch) == kUnknown);
}

static void test_teachin_implausible() {
  FaceModel d = defaultModel();
  {  // Reihenfolge im Ring vertauscht -> Seite wird nicht angenommen, Teach-in haengt dort
    TeachIn t;
    t.start();
    int order[kFaces];
    for (int i = 0; i < kFaces; ++i) order[i] = i;
    order[kRingFirst + 1] = kRingFirst + 2;
    order[kRingFirst + 2] = kRingFirst + 1;
    for (int i = 0; i < kFaces; ++i) t.offer(d.up[order[i]]);
    CHECK(!t.done());
    CHECK(t.captured() == kRingFirst + 2);
    CHECK(!t.plausible());
  }
  if (kCoin) {  // Randseiten 20 Grad schraeg (Muenze lag schief) -> Teach-in verwirft
    TeachIn t;
    t.start();
    t.offer(d.up[0]);
    for (int k = 1; k < kFaces; ++k) t.offer(rot(d.up[k], Vec3{-d.up[k].y, d.up[k].x, 0}, 20.f));
    CHECK(t.done());
    CHECK(!t.plausible());
  }
}

static void test_still_detector() {
  StillDetector sd(10, 3.f);
  Vec3 g{0, 0, 1};
  for (int i = 0; i < 9; ++i) sd.add(g);
  CHECK(!sd.still());  // Fenster noch nicht voll
  sd.add(g);
  CHECK(sd.still());
  sd.add(rot(g, Vec3{1, 0, 0}, 10.f));  // Ruck
  CHECK(!sd.still());
  for (int i = 0; i < 10; ++i) sd.add(Vec3{0, 0, 1.6f});  // Stoss (1.6 g)
  CHECK(!sd.still());
  for (int i = 0; i < 10; ++i) sd.add(rot(g, Vec3{1, 0, 0}, 1.f * (i % 2)));
  CHECK(sd.still());
}

static void test_tracker() {
  Tracker tr(3000);
  tr.restore(2);
  uint32_t t = 1000;
  CHECK(!tr.update(2, true, t));
  CHECK(tr.settled());                     // gleiche Seite -> sofort "erledigt"
  CHECK(!tr.update(4, false, t += 100));   // Bewegung
  CHECK(!tr.settled());
  CHECK(!tr.update(4, true, t += 100));
  CHECK(!tr.update(4, true, t += 2000));
  CHECK(!tr.update(3, true, t += 100));    // kippelt -> Neustart
  CHECK(!tr.update(4, true, t += 100));
  CHECK(!tr.update(4, true, t += 2999));
  CHECK(tr.update(4, true, t += 1));       // 3000 ms ruhig -> uebernehmen
  CHECK(tr.committed() == 4 && tr.settled());
  CHECK(!tr.update(4, true, t += 5000));   // kein zweites Ereignis
  // millis()-Ueberlauf
  Tracker w(3000);
  w.restore(1);
  uint32_t u = 0xFFFFF000u;
  w.update(5, true, u);
  CHECK(!w.update(5, true, u + 2000));
  CHECK(w.update(5, true, u + 3500));      // ueber 0 hinweg
}

static time_t mk(int y, int mo, int d, int h, int mi, int s = 0) {
  struct tm t{};
  t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d;
  t.tm_hour = h; t.tm_min = mi; t.tm_sec = s; t.tm_isdst = -1;
  return mktime(&t);
}

static void test_accrue() {
  DayTotals d;
  clearTotals(d);
  accrue(d, 1, mk(2026, 10, 12, 9, 0), mk(2026, 10, 12, 10, 30));
  accrue(d, 2, mk(2026, 10, 12, 10, 30), mk(2026, 10, 12, 11, 0));
  CHECK(d.sec[1] == 5400 && d.sec[2] == 1800);
  CHECK(isSameDay(d, mk(2026, 10, 12, 23, 59)));
  // ueber Mitternacht: nur der neue Tag bleibt
  accrue(d, 3, mk(2026, 10, 12, 23, 0), mk(2026, 10, 13, 0, 45));
  CHECK(d.sec[1] == 0 && d.sec[3] == 45 * 60);
  CHECK(isSameDay(d, mk(2026, 10, 13, 8, 0)));
  // Zeitumstellung 25.10.2026 (MESZ -> MEZ): 00:00 bis 06:00 sind 7 h
  DayTotals e;
  clearTotals(e);
  accrue(e, 4, mk(2026, 10, 25, 0, 0), mk(2026, 10, 25, 6, 0));
  CHECK(e.sec[4] == 7 * 3600);
  char b[16];
  formatHM(b, sizeof b, 3929);
  CHECK(strcmp(b, "1:05") == 0);
  formatHM(b, sizeof b, 29);
  CHECK(strcmp(b, "0:00") == 0);
}

static void test_csv() {
  char b[160];
  size_t n = formatCsvLine(b, sizeof b, mk(2026, 10, 12, 9, 5, 7), true, 3, 3, "Kunde, A", 3987);
  printf("  CSV: %s", b);
  CHECK(n > 0 && b[n - 1] == '\n');
  CHECK(strncmp(b, "2026-10-12T09:05:07+0200,", 25) == 0);
  CHECK(strstr(b, ",3,3,Kunde  A,3987,1\n") != nullptr);
  n = formatCsvLine(b, sizeof b, 12, false, -1, 0, "Pause", 4100);
  CHECK(strcmp(b, ",12,-1,0,Pause,4100,0\n") == 0);
}

int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  test_layout();
  test_classify_default();
  test_display_visible();
  test_up_quadrant();
  test_teachin_and_classify();
  test_teachin_implausible();
  test_still_detector();
  test_tracker();
  test_accrue();
  test_csv();
  printf("%d bestanden, %d fehlgeschlagen\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}

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

static constexpr float kMatch = 0.4f * kStepDeg;  // Fangbereich der alten classify()

static void test_layout() {
#if defined(PRISMA_PRISMA10)
  CHECK(!kCoin && kFaces == 10 && kRing == 10 && fabsf(kStepDeg - 36.f) < 1e-4f);
#else
  CHECK(kCoin && kFaces == PRISMA_RING + 1 && kRing == PRISMA_RING);
  CHECK(fabsf(kStepDeg * kRing - 360.f) < 1e-3f);
#endif
  printf("  Bauform: %s, %d Seiten, Hysterese %.1f Grad\n", kCoin ? "Muenze" : "Prisma", kFaces,
         defaultRingParams().hystDeg);
  CHECK(kMatch < kStepDeg / 2);  // Fangbereiche der alten classify() ueberlappen nicht
  CHECK(defaultRingParams().hystDeg < kStepDeg / 2);  // Hysterese kleiner als der halbe Seitenabstand
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

// ---------------------------------------------------------------- Ringerkennung (classifyRing)
static Vec3 cr(const Vec3& a, const Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Vorzeichen der Drehung um 'ax', die von Ringseite 0 zur naechsten fuehrt (haengt von der Achsrichtung ab)
static float ringSign(const FaceModel& m, const Vec3& ax) {
  const Vec3& a = m.up[kRingFirst];
  const Vec3& n = m.up[kRingFirst + 1];
  return angleDeg(rot(a, ax, kStepDeg), n) < angleDeg(rot(a, ax, -kStepDeg), n) ? 1.f : -1.f;
}

// Lage in der Ringebene, 'ang' Grad hinter der ersten Ringseite, in Richtung der naechsten Seite
static Vec3 atRing(const FaceModel& m, const Vec3& ax, float sgn, float ang) {
  return rot(m.up[kRingFirst], ax, sgn * ang);
}

static bool isRingFace(int f) { return f >= kRingFirst && f < kRingFirst + kRing; }

// exact: Seiten gleichmaessig verteilt (Standardmodell), dann sind auch die Schaltwinkel genau vorhersagbar
static void ring_checks(const FaceModel& m, const Vec3& ax, bool exact) {
  const RingParams p = defaultRingParams();
  const float sg = ringSign(m, ax);

  // 1) keine Totzone, kein Flackern: zweimal rundherum, die erkannte Seite wird mitgefuehrt
  int cur = kUnknown, bad = 0, flips = 0;
  for (float ang = 0.f; ang < 720.f; ang += 0.25f) {
    int f = classifyRing(m, atRing(m, ax, sg, ang), cur, p);
    if (!isRingFace(f)) ++bad;
    if (f != cur) ++flips;
    cur = f;
  }
  CHECK(bad == 0);
  CHECK(exact ? flips == 2 * kRing + 1 : flips <= 2 * kRing + 1);  // ein Wechsel je Seitengrenze und Runde

  // 2) auf einer Seite liegend wird genau diese erkannt, auch wenn die bisherige Seite die Nachbarin war
  for (int k = kRingFirst; k < kRingFirst + kRing; ++k) {
    CHECK(classifyRing(m, m.up[k], kUnknown, p) == k);
    CHECK(classifyRing(m, m.up[k], kRingFirst + (k - kRingFirst + 1) % kRing, p) == k);
  }

  // 3) Hysterese: hin und zurueck schalten bei verschiedenen Winkeln, Abstand = hystDeg
  if (exact) {
    for (int i = 0; i < kRing; ++i) {
      const int fi = kRingFirst + i, fn = kRingFirst + (i + 1) % kRing;
      const float base = i * kStepDeg;
      float up = -1.f, down = -1.f;
      for (float d = 0.f; d <= kStepDeg; d += 0.05f)
        if (classifyRing(m, atRing(m, ax, sg, base + d), fi, p) != fi) { up = d; break; }
      for (float d = kStepDeg; d >= 0.f; d -= 0.05f)
        if (classifyRing(m, atRing(m, ax, sg, base + d), fn, p) != fn) { down = d; break; }
      CHECK(fabsf(up - (kStepDeg / 2 + p.hystDeg / 2)) < 0.2f);
      CHECK(fabsf(down - (kStepDeg / 2 - p.hystDeg / 2)) < 0.2f);
      CHECK(up - down > p.hystDeg - 0.3f);
    }
  }

  // 4) Kippen aus der Ringebene: Eintritt bis uprightDeg, Verbleib bis uprightExitDeg
  const int f2 = kRingFirst + 2;
  const Vec3 c2 = atRing(m, ax, sg, 2 * kStepDeg);
  const Vec3 ta = cr(ax, c2);  // Kippachse in der Ebene, senkrecht zur Seitenrichtung
  for (float t : {-30.f, -10.f, 10.f, 30.f}) CHECK(classifyRing(m, rot(c2, ta, t), kUnknown, p) == f2);
  for (float t : {-40.f, 40.f}) {
    CHECK(classifyRing(m, rot(c2, ta, t), kUnknown, p) == kUnknown);  // Eintritt erst ab 35 Grad Abstand
    CHECK(classifyRing(m, rot(c2, ta, t), f2, p) == f2);              // Verbleib bis 45 Grad
  }
  CHECK(classifyRing(m, rot(c2, ta, 52.f), f2, p) == kUnknown);       // jenseits 45 Grad: raus
  CHECK(classifyRing(m, Vec3{0, 0, 0}, f2, p) == kUnknown);           // kein Messwert

  if (kCoin) {
    const Vec3 up0 = m.up[0];
    const Vec3 down{-up0.x, -up0.y, -up0.z};
    const Vec3 ra = m.up[kRingFirst];  // Kippachse in der Ringebene
    CHECK(classifyRing(m, up0, kUnknown, p) == 0);                          // auf dem Ruecken
    CHECK(classifyRing(m, down, kUnknown, p) == kUnknown);                  // Display unten
    CHECK(classifyRing(m, down, kRingFirst, p) == kUnknown);
    CHECK(classifyRing(m, rot(up0, ra, 25.f), kUnknown, p) == 0);           // Eintritt bis 30 Grad
    CHECK(classifyRing(m, rot(up0, ra, 35.f), kUnknown, p) == kUnknown);
    CHECK(classifyRing(m, rot(up0, ra, 35.f), 0, p) == 0);                  // Verbleib bis 40 Grad
    CHECK(classifyRing(m, rot(up0, ra, 45.f), 0, p) == kUnknown);
  } else {
    const Vec3 nax{-ax.x, -ax.y, -ax.z};
    CHECK(classifyRing(m, ax, kUnknown, p) == kUnknown);  // aufrecht stehend = Pause
    CHECK(classifyRing(m, ax, 3, p) == kUnknown);
    CHECK(classifyRing(m, nax, 3, p) == kUnknown);
    CHECK(classifyRing(m, m.up[0], kUnknown, p) == 0);    // Seite 0 gehoert zum Ring
  }
}

static void test_ring() {
  const FaceModel d = defaultModel();
  ring_checks(d, kCoin ? Vec3{0, 0, 1} : Vec3{0, 1, 0}, true);

  // angelerntes Modell mit schiefer Sensorlage, wie im Betrieb
  Sim s = makeSim();
  TeachIn t;
  t.start();
  CHECK(t.offer(s.face(0, 1.5f)));
  for (int k = 1; k < kFaces; ++k) CHECK(t.offer(s.face(k, (k % 2) ? 2.f : -2.5f)));
  CHECK(t.done());
  const FaceModel m = t.result();
  ring_checks(m, s.axis, false);

  std::mt19937 rng(7);
  std::normal_distribution<float> noise(0.f, 0.02f);
  const RingParams p = defaultRingParams();
  int ok = 0, tot = 0;
  for (int k = kRingFirst; k < kRingFirst + kRing; ++k)
    for (int i = 0; i < 200; ++i) {
      Vec3 a = s.face(k, noise(rng) * 100.f);  // 2 Grad Lagefehler (1 sigma)
      a.x += noise(rng); a.y += noise(rng); a.z += noise(rng);  // 20 mg Sensorrauschen
      ok += classifyRing(m, a, kUnknown, p) == k;
      ++tot;
    }
  printf("  Ringerkennung mit Rauschen: %d/%d korrekt, Hysterese %.1f Grad\n", ok, tot, p.hystDeg);
  CHECK(ok >= tot * 995 / 1000);
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

// ---------------------------------------------------------------- Schuetteln
// Parameter wie in config.h (SHAKE_JERK_G, SHAKE_PEAKS, SHAKE_WINDOW_MS)
static ShakeDetector makeShake() { return ShakeDetector(0.25f, 4, 1000); }

// Sinus auf der X-Achse zusaetzlich zur Schwerkraft; liefert den Zeitpunkt (ms) der ersten Erkennung oder -1
static int feedSine(ShakeDetector& sd, float ampG, float hz, int ms, int* hits = nullptr) {
  int first = -1, n = 0;
  for (int t = 0; t < ms; t += 20) {
    Vec3 a{ampG * sinf(2.0f * 3.14159265f * hz * t / 1000.0f), 0.0f, 1.0f};
    if (sd.add(a, (uint32_t)t)) {
      if (first < 0) first = t;
      ++n;
    }
  }
  if (hits) *hits = n;
  return first;
}

// Stoss: Samples mit z = 1 + amp[i], dazwischen ruhig (20 ms Takt ab startMs)
static bool feedKnocks(ShakeDetector& sd, int knocks, int spacingMs) {
  bool any = false;
  uint32_t t = 0;
  for (int i = 0; i < 20; ++i, t += 20) any |= sd.add({0, 0, 1}, t);  // ruhig
  for (int k = 0; k < knocks; ++k) {
    any |= sd.add({0, 0, 3.0f}, t); t += 20;   // Anstoss
    any |= sd.add({0, 0, 0.2f}, t); t += 20;   // Rueckprall
    for (int i = 0; i < spacingMs / 20 - 2; ++i, t += 20) any |= sd.add({0, 0, 1}, t);
  }
  for (int i = 0; i < 20; ++i, t += 20) any |= sd.add({0, 0, 1}, t);
  return any;
}

static void test_shake() {
  ShakeDetector sd = makeShake();
  int t = feedSine(sd, 1.0f, 3.0f, 3000);
  printf("  Schuetteln 1,0 g / 3 Hz erkannt nach %d ms\n", t);
  CHECK(t >= 0 && t <= 800);
  sd.reset();
  t = feedSine(sd, 2.0f, 5.0f, 3000);
  CHECK(t >= 0 && t <= 600);
  sd.reset();
  t = feedSine(sd, 1.5f, 2.0f, 3000);
  printf("  Schuetteln 1,5 g / 2 Hz erkannt nach %d ms\n", t);
  CHECK(t >= 0 && t <= 1300);
  // dauerhaftes Schuetteln loest immer wieder aus (Zaehlung beginnt nach jeder Erkennung neu)
  sd.reset();
  int hits = 0;
  feedSine(sd, 1.5f, 3.0f, 4000, &hits);
  CHECK(hits >= 3);
  // sanftes Wackeln bleibt unter der Schwelle
  sd.reset();
  CHECK(feedSine(sd, 0.3f, 3.0f, 5000) < 0);
  sd.reset();
  CHECK(feedSine(sd, 1.5f, 0.5f, 6000) < 0);  // langsames Hin und Her (0,5 Hz)

  // Rauschen
  std::mt19937 rng(7);
  std::uniform_real_distribution<float> n(-0.02f, 0.02f);
  sd.reset();
  bool any = false;
  for (int k = 0; k < 500; ++k) any |= sd.add({n(rng), n(rng), 1.0f + n(rng)}, (uint32_t)(k * 20));
  CHECK(!any);

  // Drehen: 90 Grad in 0,5 s, 360 Grad in 1,5 s, schnelles Umdrehen 180 Grad in 0,3 s
  struct Rot { float deg; int ms; } rots[] = {{90, 500}, {360, 1500}, {180, 300}};
  for (auto& r : rots) {
    sd.reset();
    any = false;
    for (int t2 = 0; t2 <= r.ms; t2 += 20) {
      float th = r.deg * (float)t2 / (float)r.ms * (3.14159265f / 180.0f);
      any |= sd.add({0, sinf(th), cosf(th)}, (uint32_t)t2);
    }
    CHECK(!any);
  }

  // einzelne Stoesse
  sd = makeShake();
  CHECK(!feedKnocks(sd, 1, 400));
  sd = makeShake();
  CHECK(!feedKnocks(sd, 2, 200));
  sd = makeShake();
  CHECK(!feedKnocks(sd, 3, 200));
  // vier Klopfer im Abstand von 300 ms liegen im 1-s-Fenster, im Abstand von 400 ms nicht
  sd = makeShake();
  CHECK(feedKnocks(sd, 4, 300));
  sd = makeShake();
  CHECK(!feedKnocks(sd, 4, 400));

  // Luecke zwischen den Samples: grosse Spruenge ueber 300 ms zaehlen nicht
  sd = makeShake();
  any = false;
  for (int k = 0; k < 40; ++k) any |= sd.add({(k % 2) ? 2.0f : -2.0f, 0, 1}, (uint32_t)(k * 300));
  CHECK(!any);
}

static void test_daylog() {
  char b[48];
  dayLogPath(b, sizeof b, mk(2026, 10, 29, 9, 5, 7), true);
  CHECK(strcmp(b, "/log_2026-10-29.csv") == 0);
  dayLogPath(b, sizeof b, mk(2026, 1, 2, 23, 59, 59), true);
  CHECK(strcmp(b, "/log_2026-01-02.csv") == 0);
  dayLogPath(b, sizeof b, 12345, false);
  CHECK(strcmp(b, "/log_ohne_zeit.csv") == 0);
  // lokale Mitternacht, auch am Tag der Zeitumstellung (25.10.2026 hat 25 Stunden)
  CHECK(localMidnight(mk(2026, 10, 29, 9, 5, 7)) == mk(2026, 10, 29, 0, 0, 0));
  CHECK(localMidnight(mk(2026, 10, 29, 0, 0, 0)) == mk(2026, 10, 29, 0, 0, 0));
  CHECK(localMidnight(mk(2026, 10, 29, 23, 59, 59)) == mk(2026, 10, 29, 0, 0, 0));
  CHECK(localMidnight(mk(2026, 10, 25, 12, 0)) == mk(2026, 10, 25, 0, 0));
  CHECK(mk(2026, 10, 26, 0, 0) - localMidnight(mk(2026, 10, 25, 12, 0)) == 25 * 3600);
  // Namen sortieren zeitlich (Grundlage fuer den gemeinsamen Export)
  char a1[48], a2[48];
  dayLogPath(a1, sizeof a1, mk(2026, 10, 28, 12, 0), true);
  dayLogPath(a2, sizeof a2, mk(2026, 10, 29, 12, 0), true);
  CHECK(strcmp(a1, a2) < 0);
  dayLogPath(a1, sizeof a1, 0, false);
  CHECK(strcmp(a2, a1) < 0);  // "ohne_zeit" kommt zuletzt
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
  test_ring();
  test_still_detector();
  test_tracker();
  test_accrue();
  test_csv();
  test_shake();
  test_daylog();
  printf("%d bestanden, %d fehlgeschlagen\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}

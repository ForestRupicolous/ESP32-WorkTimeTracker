// Hardware-Schicht StickS3
//
// Weckkette laut M5-Doku "StickS3 Low-Power Configuration":
//   BMI270 INT1 -> M5PM1 PYG4 (Eingang)  ->  M5PM1 PYG1 als IRQ-Ausgang -> ESP32-S3 G13
// Der ESP32-S3 schlaeft im Deep Sleep (Board-Zustand L2), IMU und M5PM1 bleiben versorgt.
//
// Alles laeuft ueber den M5Unified-I2C-Bus (M5.In_I2C). Kein zweiter Treiber
// (Arduino-Wire) auf denselben Pins: lgfx-I2C legt die Pins bei jeder Transaktion
// neu auf seinen Port, Wire wuerde danach ins Leere schreiben.
#include "hal.h"

#include <M5PM1.h>
#include <M5Unified.h>
#include <driver/rtc_io.h>

#include "config.h"

namespace {

M5PM1 pm1;
bool pm1Ok = false;
uint8_t bmiAddr = 0;

constexpr uint32_t kI2cHz = 400000;
constexpr gpio_num_t PIN_PM1_IRQ = GPIO_NUM_13;  // M5PM1 PYG1_IRQ
constexpr gpio_num_t PIN_BTN_A = GPIO_NUM_11;    // KEY1

// BMI270-Register (Bosch BMI270 SensorAPI, bmi2_defs.h / bmi270.h)
constexpr uint8_t REG_CHIP_ID = 0x00;
constexpr uint8_t REG_FEAT_PAGE = 0x2F;
constexpr uint8_t REG_FEATURES = 0x30;      // 16-Byte-Fenster der Feature-Seite
constexpr uint8_t REG_ACC_CONF = 0x40;
constexpr uint8_t REG_INT1_IO_CTRL = 0x53;
constexpr uint8_t REG_INT_LATCH = 0x55;
constexpr uint8_t REG_INT1_MAP_FEAT = 0x56;
constexpr uint8_t REG_INT2_IO_CTRL = 0x54;
constexpr uint8_t REG_INT2_MAP_FEAT = 0x57;
constexpr uint8_t REG_INT_MAP_DATA = 0x58;
constexpr uint8_t REG_PWR_CONF = 0x7C;
constexpr uint8_t REG_PWR_CTRL = 0x7D;
constexpr uint8_t BMI270_ID = 0x24;
constexpr uint8_t ANYMOT_PAGE = 1;          // BMI2_PAGE_1
constexpr uint8_t ANYMOT_OFS = 0x0C;        // BMI270_ANY_MOT_STRT_ADDR
constexpr uint8_t ANYMOT_INT = 0x40;        // BMI270_INT_ANY_MOT_MASK

bool bmiW(uint8_t reg, uint8_t v) {
  bool ok = M5.In_I2C.writeRegister8(bmiAddr, reg, v, kI2cHz);
  delayMicroseconds(500);  // >= 450 us falls Advanced Power Save aktiv
  return ok;
}
bool bmiR(uint8_t reg, uint8_t* d, size_t n) { return M5.In_I2C.readRegister(bmiAddr, reg, d, n, kI2cHz); }

bool detectBmi() {
  for (uint8_t a : {uint8_t(0x68), uint8_t(0x69)}) {
    if (M5.In_I2C.readRegister8(a, REG_CHIP_ID, kI2cHz) == BMI270_ID) { bmiAddr = a; return true; }
  }
  return false;
}

bool bmiLowPower(bool advPowerSave) {
  bool ok = bmiW(REG_PWR_CONF, 0x00);   // APS aus, damit Konfiguration sicher greift
  ok &= bmiW(REG_PWR_CTRL, 0x04);       // nur Accel an, Gyro/Temp/Aux aus
  ok &= bmiW(REG_ACC_CONF, 0x17);       // 50 Hz, avg2, power-optimized
  if (advPowerSave) ok &= bmiW(REG_PWR_CONF, 0x01);
  return ok;
}

// any-motion auf INT1, push-pull, low-aktiv, nicht gelatcht (wie M5-Beispiel)
bool bmiArmAnyMotion() {
  bool ok = bmiW(REG_PWR_CONF, 0x00);
  ok &= bmiW(REG_FEAT_PAGE, ANYMOT_PAGE);
  uint8_t f[16];
  ok &= bmiR(REG_FEATURES, f, sizeof f);
  if (!ok) return false;
  uint16_t w1 = f[ANYMOT_OFS + 2] | (f[ANYMOT_OFS + 3] << 8);
  uint16_t w0 = (ANYMOT_DURATION & 0x1FFF) | 0xE000;        // Dauer + X/Y/Z-Auswahl
  w1 = (w1 & 0x7800) | (ANYMOT_THRESHOLD & 0x07FF) | 0x8000;  // Schwelle + Enable (Bit 15)
  f[ANYMOT_OFS] = w0 & 0xFF;
  f[ANYMOT_OFS + 1] = w0 >> 8;
  f[ANYMOT_OFS + 2] = w1 & 0xFF;
  f[ANYMOT_OFS + 3] = w1 >> 8;
  ok &= M5.In_I2C.writeRegister(bmiAddr, REG_FEATURES, f, sizeof f, kI2cHz);
  delayMicroseconds(500);
  ok &= bmiW(REG_INT1_IO_CTRL, 0x08);   // output_en=1, od=push-pull, lvl=low
  ok &= bmiW(REG_INT_LATCH, 0x00);
  ok &= bmiW(REG_INT1_MAP_FEAT, ANYMOT_INT);
  // Nur any-motion darf INT1 treiben. Am Geraet stand INT_MAP_DATA nach M5.begin() auf 0xFF (Daten-bereit-,
  // FIFO- und Fehler-Interrupts auf INT1 und INT2): INT1 pulste mit jedem Messwert (50 Hz) und weckte den Stick dauernd.
  ok &= bmiW(REG_INT_MAP_DATA, 0x00);
  ok &= bmiW(REG_INT2_MAP_FEAT, 0x00);
  ok &= bmiW(REG_INT2_IO_CTRL, 0x00);
  ok &= bmiLowPower(true);
  return ok;
}

void pm1ClearIrq() {
  pm1.irqClearGpioAll();
  pm1.irqClearSysAll();
  pm1.irqClearBtnAll();
}

// IMU-INT (PYG4) -> IRQ-Ausgang PYG1 -> ESP G13 (Code aus M5-Doku)
void pm1ArmImuIrq() {
  pm1ClearIrq();
  pm1.irqSetGpioMaskAll(M5PM1_IRQ_MASK_ENABLE);
  pm1.irqSetSysMaskAll(M5PM1_IRQ_MASK_ENABLE);
  pm1.irqSetBtnMaskAll(M5PM1_IRQ_MASK_ENABLE);
  pm1.irqSetGpioMask(M5PM1_IRQ_GPIO4, M5PM1_IRQ_MASK_DISABLE);
  pm1.gpioSetMode(M5PM1_GPIO_NUM_4, M5PM1_GPIO_MODE_INPUT);
  pm1.gpioSetPull(M5PM1_GPIO_NUM_4, M5PM1_GPIO_PULL_UP);
  pm1.gpioSetMode(M5PM1_GPIO_NUM_1, M5PM1_GPIO_MODE_OUTPUT);
  pm1.gpioSetDrive(M5PM1_GPIO_NUM_1, M5PM1_GPIO_DRIVE_PUSHPULL);
  pm1.gpioSetFunc(M5PM1_GPIO_NUM_1, M5PM1_GPIO_FUNC_IRQ);
}

// L3B (Backlight, Mikrofon, Lautsprecher) ueber PYG2 abschalten (Code aus M5-Doku).
// M5.begin() schaltet L3B beim naechsten Start wieder ein.
void powerOffL3B() {
  pm1.gpioSetFunc(M5PM1_GPIO_NUM_2, M5PM1_GPIO_FUNC_GPIO);
  pm1.gpioSetMode(M5PM1_GPIO_NUM_2, M5PM1_GPIO_MODE_OUTPUT);
  pm1.gpioSetDrive(M5PM1_GPIO_NUM_2, M5PM1_GPIO_DRIVE_PUSHPULL);
  pm1.gpioSetOutput(M5PM1_GPIO_NUM_2, false);
}

// Nach dem Umkonfigurieren des BMI270 loest die any-motion-Erkennung einmal aus (am 29.10.2026 gemessen: ca.
// 130 ms nach dem Armieren, bei ruhig liegendem Stick). Sofort loeschen und schlafen weckt den Stick darum
// gleich wieder. Deshalb wach bleiben, den IRQ bei jeder Ausloesung loeschen und erst schlafen, wenn G13
// quietMs lang ruhig HIGH war. false = kommt nicht zur Ruhe (echte Bewegung), dann Timer-Rueckfall.
bool waitIrqQuiet(uint32_t quietMs, uint32_t maxMs) {
  uint32_t t0 = millis(), tq = t0;
  int clears = 0;
  while (millis() - t0 < maxMs) {
    if (digitalRead(PIN_PM1_IRQ) == LOW) {
      pm1ClearIrq();
      ++clears;
      tq = millis();
#if WAKE_DIAG
      if (clears <= 3) log_i("DIAG Ausloesung %d nach %u ms, IRQ geloescht", clears, (unsigned)(tq - t0));
#endif
    } else if (millis() - tq >= quietMs) {
#if WAKE_DIAG
      log_i("DIAG ruhig nach %u ms, %d Ausloesung(en)", (unsigned)(millis() - t0), clears);
#endif
      return true;
    }
    delay(2);
  }
#if WAKE_DIAG
  log_i("DIAG nicht zur Ruhe gekommen: %d Ausloesungen in %u ms", clears, (unsigned)maxMs);
#endif
  return false;
}

#if WAKE_DIAG
// ---- Diagnose der Weckkette ----
// Beim Wecken lesen, BEVOR die Status geloescht werden: was hat G13 nach unten gezogen?
struct Diag {
  bool valid;
  uint8_t g13;                   // ESP G13 (M5PM1-IRQ-Ausgang PYG1)
  uint8_t pmGpio, pmSys, pmBtn;  // M5PM1 IRQ-Status: GPIO / System / Taste
  uint8_t in1, in4;              // M5PM1 PYG1 (IRQ-Ausgang) und PYG4 (BMI270 INT1), Eingangspegel
  uint8_t bmi0, bmi1;            // BMI270 INT_STATUS_0 / 1 (Bit 6 von int0 = any-motion)
} diag;

void readStatus(uint8_t* g, uint8_t* s, uint8_t* b, uint8_t* i1, uint8_t* i4, uint8_t* b0, uint8_t* b1) {
  *g = *s = *b = *i1 = *i4 = *b0 = *b1 = 0;
  if (pm1Ok) {
    pm1.irqGetGpioStatus(g);
    pm1.irqGetSysStatus(s);
    pm1.irqGetBtnStatus(b);
    pm1.gpioGetInput(M5PM1_GPIO_NUM_1, i1);
    pm1.gpioGetInput(M5PM1_GPIO_NUM_4, i4);
  }
  uint8_t st[2] = {0, 0};
  if (bmiAddr && bmiR(0x1C, st, 2)) { *b0 = st[0]; *b1 = st[1]; }
}

void diagCapture() {
  pinMode(PIN_PM1_IRQ, INPUT_PULLUP);
  diag.g13 = digitalRead(PIN_PM1_IRQ);
  readStatus(&diag.pmGpio, &diag.pmSys, &diag.pmBtn, &diag.in1, &diag.in4, &diag.bmi0, &diag.bmi1);
  diag.valid = true;
}

// BMI270-Beschleunigung roh lesen und in mg umrechnen (Messbereich aus ACC_RANGE 0x41)
bool bmiAccMg(float mg[3]) {
  uint8_t r = 0, d[6];
  if (!bmiR(0x41, &r, 1) || !bmiR(0x0C, d, 6)) return false;
  float lsbMg = 1000.f * (2 << (r & 3)) / 32768.f;
  for (int i = 0; i < 3; ++i) mg[i] = (int16_t)(d[2 * i] | (d[2 * i + 1] << 8)) * lsbMg;
  return true;
}

// Nach dem Armieren ms lang wach bleiben: Pegel von G13 beobachten, bei jeder Aenderung den Status loggen
void diagWatch(uint32_t ms) {
  uint32_t t0 = millis();
  int last = digitalRead(PIN_PM1_IRQ);
  float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
  log_i("DIAG armiert: G13=%d", last);
  while (millis() - t0 < ms) {
    float a[3];
    if (bmiAccMg(a))
      for (int i = 0; i < 3; ++i) {
        if (a[i] < mn[i]) mn[i] = a[i];
        if (a[i] > mx[i]) mx[i] = a[i];
      }
    int v = digitalRead(PIN_PM1_IRQ);
    if (v != last) {
      last = v;
      uint8_t g, s, b, i1, i4, b0, b1;
      readStatus(&g, &s, &b, &i1, &i4, &b0, &b1);
      log_i("DIAG t=%u ms: G13=%d PM1 irq g=%02X s=%02X b=%02X in1=%d in4=%d | BMI int0=%02X int1=%02X",
            (unsigned)(millis() - t0), v, g, s, b, i1, i4, b0, b1);
    }
    delay(5);
  }
  log_i("DIAG Ende nach %u ms: G13=%d, Hub in mg: x=%.0f y=%.0f z=%.0f", (unsigned)ms, last,
        mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]);
}

// BMI270-Konfiguration aus dem Chip zurueckpruefen (Register und any-motion-Woerter)
void diagDumpBmi() {
  auto rd = [](uint8_t reg) { uint8_t v = 0xEE; bmiR(reg, &v, 1); return v; };
  log_i("DIAG BMI: ACC_CONF=%02X ACC_RANGE=%02X INT1_IO=%02X INT_LATCH=%02X INT1_MAP_FEAT=%02X INT_MAP_DATA=%02X "
        "PWR_CONF=%02X PWR_CTRL=%02X INTERNAL_STATUS=%02X",
        rd(0x40), rd(0x41), rd(0x53), rd(0x55), rd(0x56), rd(0x58), rd(0x7C), rd(0x7D), rd(0x21));
  // Feature-Register sind nur bei ausgeschaltetem Advanced-Power-Save lesbar (sonst Nullen)
  uint8_t f[16] = {0};
  bmiW(REG_PWR_CONF, 0x00);
  bmiW(REG_FEAT_PAGE, ANYMOT_PAGE);
  bmiR(REG_FEATURES, f, sizeof f);
  bmiW(REG_PWR_CONF, 0x01);
  log_i("DIAG BMI any-motion: w0=%04X (Dauer/Achsen) w1=%04X (Schwelle/out_conf/enable)",
        f[ANYMOT_OFS] | (f[ANYMOT_OFS + 1] << 8), f[ANYMOT_OFS + 2] | (f[ANYMOT_OFS + 3] << 8));
}

// ms lang INT1 (M5PM1 PYG4, ueber I2C gelesen) und G13 ohne Loeschen verfolgen, mit Mikrosekunden-Zeitstempel.
// Zeigt, ob INT1 wirklich pulst (BMI270) oder G13 ohne INT1 faellt (M5PM1).
void diagTraceInt(uint32_t ms) {
  struct Ev { uint32_t us; uint8_t src, val; } ev[120];
  int n = 0;
  uint32_t reads = 0, t0 = micros();
  int lastG = digitalRead(PIN_PM1_IRQ);
  uint8_t last4 = 2, v = 0;
  while (micros() - t0 < ms * 1000UL && n < 118) {
    ++reads;
    if (pm1.gpioGetInput(M5PM1_GPIO_NUM_4, &v) == M5PM1_OK && v != last4) { ev[n++] = {micros() - t0, 4, v}; last4 = v; }
    int g = digitalRead(PIN_PM1_IRQ);
    if (g != lastG) { ev[n++] = {micros() - t0, 13, (uint8_t)g}; lastG = g; }
  }
  log_i("DIAG Trace: %u PM1-Lesungen in %u ms, %d Ereignisse, G13 am Ende %d", (unsigned)reads, (unsigned)ms, n, lastG);
  for (int i = 0; i < n; ++i)
    log_i("DIAG   %7u us: %s=%d", (unsigned)ev[i].us, ev[i].src == 13 ? "G13" : "in4", ev[i].val);
}
#endif

}  // namespace

namespace hal {

bool begin() {
  pm1Ok = (pm1.begin(&M5.In_I2C) == M5PM1_OK);
  bool imuOk = detectBmi();
#if WAKE_DIAG
  diagCapture();  // IRQ-Quellen lesen, bevor sie geloescht werden
#endif
  if (pm1Ok) pm1ClearIrq();  // IRQ-Leitung nach Aufwachen freigeben
  if (!pm1Ok) log_e("M5PM1 nicht erreichbar");
  if (!imuOk) log_e("BMI270 nicht gefunden");
  return pm1Ok && imuOk;
}

bool readAccel(prisma::Vec3& a) { return M5.Imu.getAccel(&a.x, &a.y, &a.z); }

int batteryMv() {
  uint16_t mv = 0;
  if (pm1Ok && pm1.readVbat(&mv) == M5PM1_OK) return mv;
  return 0;
}

esp_sleep_wakeup_cause_t wakeCause() { return esp_sleep_get_wakeup_cause(); }

bool wokeByButton() {
  auto c = wakeCause();
  return (c == ESP_SLEEP_WAKEUP_EXT1 || c == ESP_SLEEP_WAKEUP_GPIO) && digitalRead(PIN_BTN_A) == LOW;
}

void logWakeDiag() {
#if WAKE_DIAG
  if (!diag.valid) return;
  log_i("DIAG beim Wecken (Grund %d): G13=%d PM1 irq g=%02X s=%02X b=%02X in1=%d in4=%d | BMI int0=%02X int1=%02X",
        (int)wakeCause(), diag.g13, diag.pmGpio, diag.pmSys, diag.pmBtn, diag.in1, diag.in4, diag.bmi0, diag.bmi1);
#endif
}

void imuLowPower() {
  if (bmiAddr) bmiLowPower(false);
}

void sleepUntilMotion() {
  M5.Speaker.end();
  M5.Display.sleep();
  M5.Display.waitDisplay();
  logWakeDiag();

  if (pm1Ok) powerOffL3B();
  pinMode(PIN_PM1_IRQ, INPUT_PULLUP);
  bool armed = pm1Ok && bmiAddr && bmiArmAnyMotion();
  if (armed) {
    pm1ArmImuIrq();
#if WAKE_DIAG
    diagDumpBmi();
    diagTraceInt(1500);  // INT1 (in4) und G13 ohne Loeschen beobachten
#endif
    armed = waitIrqQuiet(IRQ_QUIET_MS, IRQ_SETTLE_MAX_MS);  // BMI270 einschwingen lassen, IRQ loeschen
  }
#if WAKE_DIAG
  if (armed) diagWatch(1500);  // Kontrolle: bleibt G13 jetzt ruhig?
#endif
  if (armed && digitalRead(PIN_PM1_IRQ) == HIGH) {
    esp_sleep_enable_ext0_wakeup(PIN_PM1_IRQ, 0);
    rtc_gpio_pullup_en(PIN_PM1_IRQ);
    rtc_gpio_pulldown_dis(PIN_PM1_IRQ);
  } else {
    // Rueckfall, damit kein Dauer-Aufwachen den Akku leert
    log_w("Bewegungs-Wecken nicht verfuegbar, Timer-Rueckfall 60 s");
    esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL);
  }
  esp_sleep_enable_ext1_wakeup(1ULL << PIN_BTN_A, ESP_EXT1_WAKEUP_ANY_LOW);
#if WAKE_DIAG
  delay(150);  // USB-Ausgabe abfliessen lassen
#endif
  esp_deep_sleep_start();
  for (;;) {}
}

void sleepFor(uint32_t seconds) {
  M5.Speaker.end();
  M5.Display.sleep();
  M5.Display.waitDisplay();
  if (pm1Ok) {
    pm1ClearIrq();
    powerOffL3B();
  }
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_sleep_enable_ext1_wakeup(1ULL << PIN_BTN_A, ESP_EXT1_WAKEUP_ANY_LOW);
  esp_deep_sleep_start();
  for (;;) {}
}

void lightSleep(uint32_t ms) {
  esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);
  gpio_wakeup_enable(PIN_BTN_A, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
  gpio_wakeup_disable(PIN_BTN_A);
}

}  // namespace hal

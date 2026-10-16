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

}  // namespace

namespace hal {

bool begin() {
  pm1Ok = (pm1.begin(&M5.In_I2C) == M5PM1_OK);
  if (pm1Ok) pm1ClearIrq();  // IRQ-Leitung nach Aufwachen freigeben
  bool imuOk = detectBmi();
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

void imuLowPower() {
  if (bmiAddr) bmiLowPower(false);
}

void sleepUntilMotion() {
  M5.Speaker.end();
  M5.Display.sleep();
  M5.Display.waitDisplay();

  bool armed = pm1Ok && bmiAddr && bmiArmAnyMotion();
  if (armed) {
    pm1ArmImuIrq();
    delay(30);
    pm1ClearIrq();  // evtl. Puls beim Konfigurieren verwerfen
  }
  if (pm1Ok) powerOffL3B();

  pinMode(PIN_PM1_IRQ, INPUT_PULLUP);
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

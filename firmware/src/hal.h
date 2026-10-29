// Hardware: IMU, Akku, Schlafmodi (StickS3 mit M5PM1 + BMI270)
#pragma once
#include <esp_sleep.h>

#include "prisma_core.h"

namespace hal {

// Warmstart nach Deep Sleep: BMI270 ist noch konfiguriert, die M5Unified-IMU-Initialisierung (Reset + 8-KB-Upload,
// ca. 0,3 s) laesst sich vor M5.begin() sparen (cfg.internal_imu = false). Nur wahr, wenn der Abgleich beim letzten
// Kaltstart gepasst hat (eigene Lesung == M5.Imu).
bool warmImuReady();

// nach M5.begin(); M5PM1 + BMI270-Adresse. ownImu = M5.Imu wurde nicht gestartet, hal liest das BMI270 selbst.
bool begin(bool ownImu = false);
bool readAccel(prisma::Vec3& a);      // in g, Achsen wie M5Unified
int  batteryMv();                     // 0 bei Fehler
esp_sleep_wakeup_cause_t wakeCause();
bool wokeByButton();

// Diagnose (WAKE_DIAG): beim Wecken gelesene IRQ-Quellen ausgeben. Ohne WAKE_DIAG ein No-op.
void logWakeDiag();

// Deep Sleep bis Bewegung (IMU any-motion -> M5PM1 PYG4 -> IRQ PYG1 -> ESP G13)
// oder Taste A (G11). Kehrt nie zurueck.
[[noreturn]] void sleepUntilMotion();

// Deep Sleep nur mit Timer (+ Taste A), z. B. wenn er in der Tasche dauernd wackelt
[[noreturn]] void sleepFor(uint32_t seconds);

// Light Sleep fuer ms Millisekunden (Taste A weckt ebenfalls). Kehrt zurueck.
void lightSleep(uint32_t ms);

// Gyro aus, Accel stromsparend (fuer Light-Sleep-Modus)
void imuLowPower();

}  // namespace hal

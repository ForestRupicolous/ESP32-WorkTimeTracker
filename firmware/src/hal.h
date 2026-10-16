// Hardware: IMU, Akku, Schlafmodi (StickS3 mit M5PM1 + BMI270)
#pragma once
#include <esp_sleep.h>

#include "prisma_core.h"

namespace hal {

bool begin();                         // nach M5.begin(); M5PM1 + BMI270-Adresse
bool readAccel(prisma::Vec3& a);      // in g, Achsen wie M5Unified
int  batteryMv();                     // 0 bei Fehler
esp_sleep_wakeup_cause_t wakeCause();
bool wokeByButton();

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

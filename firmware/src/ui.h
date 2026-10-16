// Anzeige und Toene
#pragma once
#include <time.h>

#include "prisma_core.h"

namespace ui {

void begin();
void off();
void beep(int count, int freq = 2600, int ms = 70);
void longBeep();

// Bestaetigung nach Seitenwechsel (Projektname in Projektfarbe)
void showProject(int project, const char* name);
// Pause/Status: Tagessummen, laufendes Projekt, Akku
void showStatus(const prisma::DayTotals& d, int currentProject, time_t since, time_t now,
                bool timeValid, int vbatMv);
void showText(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr);

}  // namespace ui

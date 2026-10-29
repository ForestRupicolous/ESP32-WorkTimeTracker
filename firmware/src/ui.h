// Anzeige und Toene
#pragma once
#include <time.h>

#include "prisma_core.h"

namespace ui {

void begin();
void off();
void setRotation(uint8_t rot);
void beep(int count, int freq = 2600, int ms = 70);
void longBeep();
// Projektnummer als Tonfolge: je 5 ein langer Ton, Rest kurz (7 = lang + 2 kurz, 12 = 2 lang + 2 kurz)
void beepProject(int project);

// Bestaetigung nach Seitenwechsel (Projektname in Projektfarbe)
void showProject(int project, const char* name, uint8_t rot);
// Pause/Status (Rotation DISP_ROT_FLAT): Tagessummen, laufendes Projekt, Akku
void showStatus(const prisma::DayTotals& d, int currentProject, time_t since, time_t now,
                bool timeValid, int vbatMv);
void showText(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr);

}  // namespace ui

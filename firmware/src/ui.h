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
// Projekt-Ton: ein weicher Einzelton je Projekt (PROJECT_TONES_HZ in config.h, 0 = Pause). startProjectTone() kehrt
// sofort zurueck (der Ton laeuft im Hintergrund), waitTone() wartet auf das Ende (vor dem Schlafen aufrufen).
void startProjectTone(int project);
void waitTone();
void beepProject(int project);  // = startProjectTone() + waitTone()

// Lautsprecher vorab starten (braucht eine Anlaufzeit, sonst geht der erste Ton nach dem Wecken verloren)
void prepareSound();

// Bestaetigung nach Seitenwechsel: Projektname in Projektfarbe, darunter gross die aufsummierte Zeit des Projekts (h:mm):
// heute, bei noDate (keine Uhrzeit) die Summe ohne Datum mit dem Hinweis "ohne Datum". sec < 0 = unbekannt, dann "--:--".
// fsPercent = Belegung des Log-Speichers, ab FS_WARN_PERCENT erscheint unten eine Warnung.
void showProject(int project, const char* name, uint8_t rot, int32_t sec, bool noDate, int fsPercent);
// Pause/Status (Rotation DISP_ROT_FLAT): Summen je Projekt (sec[0..kFaces-1], ohne den laufenden Abschnitt), laufendes
// Projekt, Akku. timeValid = false: Kopfzeile "Ohne Datum", die Summen sind dann die Summen ohne Datum.
void showStatus(const uint32_t* sec, int currentProject, time_t since, time_t now, bool timeValid, int vbatMv,
                int fsPercent);
void showText(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr);

}  // namespace ui

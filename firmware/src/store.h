// Ablage: Ereignis-Log (LittleFS, CSV, eine Datei je Tag) und Seitenmodell (NVS)
#pragma once
#include <functional>
#include <time.h>

#include "prisma_core.h"

namespace store {

bool begin();
bool loadModel(prisma::FaceModel& m);
bool saveModel(const prisma::FaceModel& m);

// Der Abschnitt, der mit dem neuen Ereignis endet (fuer die Uebertragszeile in einer neuen Tagesdatei)
struct Carry {
  time_t since;
  int face;
  int project;
  const char* name;
};

// Ereignis in die Tagesdatei anhaengen: /log_JJJJ-MM-TT.csv (lokales Datum von t), ohne gueltige Uhrzeit
// /log_ohne_zeit.csv. Ist die Tagesdatei neu und begann der vorige Abschnitt vor Mitternacht, steht vor dem Ereignis
// eine Uebertragszeile um 00:00:00 (vbat_mv = 0), damit jede Tagesdatei fuer sich auswertbar ist.
bool appendEvent(time_t t, bool timeValid, int face, int project, const char* name, int vbatMv,
                 const Carry* carry = nullptr);

// Logdateien in zeitlicher Reihenfolge: Schluessel "alt" (/log.csv aus frueheren Staenden), "JJJJ-MM-TT", "ohne_zeit"
void listLogs(const std::function<void(const char* key, size_t size)>& cb);
size_t logSize();  // Summe aller Logdateien in Bytes
// Alle Logdateien hintereinander mit einer einzigen Kopfzeile (Export /log.csv)
void streamAll(const std::function<void(const uint8_t* data, size_t n)>& sink);
// Pfad zu einem Schluessel (streng geprueft: "alt", "ohne_zeit" oder JJJJ-MM-TT), false bei ungueltigem Argument
bool pathFromKey(const char* key, char* out, size_t n);

}  // namespace store

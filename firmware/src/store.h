// Ablage: Ereignis-Log (LittleFS, CSV) und Seitenmodell (NVS)
#pragma once
#include <time.h>

#include "prisma_core.h"

namespace store {

bool begin();
bool loadModel(prisma::FaceModel& m);
bool saveModel(const prisma::FaceModel& m);
bool appendEvent(time_t t, bool timeValid, int face, int project, const char* name, int vbatMv);
const char* logPath();
size_t logSize();

}  // namespace store

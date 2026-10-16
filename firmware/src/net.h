// WLAN: Zeitsync per NTP und lokaler Export (http://zeitprisma.local)
#pragma once
#include <Arduino.h>
#include <functional>
#include <time.h>

namespace net {

// Verbindet, synchronisiert, trennt. true bei gueltiger Zeit.
bool syncTime();

// Webserver bis Timeout oder keepRunning() == false. statusHtml liefert die Startseite.
void serveExport(const std::function<String()>& statusHtml,
                 const std::function<bool()>& keepRunning,
                 const std::function<void(const char* ip)>& onReady);

}  // namespace net

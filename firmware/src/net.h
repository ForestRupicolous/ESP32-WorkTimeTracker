// WLAN: Zeitsync per NTP und lokaler Export (http://zeitprisma.local)
#pragma once
#include <Arduino.h>
#include <functional>
#include <time.h>

namespace net {

// Zeitabgleich im Hintergrund (eigene Task): Scan, Verbindung und NTP blockieren die Hauptschleife nicht.
// syncStart(): startet den Abgleich, false wenn schon einer laeuft oder die Task nicht angelegt werden konnte.
// syncRunning(): true, solange die Task laeuft (nicht schlafen gehen).
// takeClockSet(): true (einmal), sobald der SNTP-Dienst die Uhr gestellt hat; die Uhr springt dann schon, die Hauptschleife
//   kann den Sprung sofort uebernehmen, ohne das Ende der Task abzuwarten.
// syncFinished(&ok): true (einmal), sobald die Task beendet ist; ok = Uhrzeit erhalten.
// syncCancel(): bricht ab und wartet auf das Ende (fuer den Export, der die WLAN-Schnittstelle selbst braucht).
bool syncStart();
bool syncRunning();
bool takeClockSet();
bool syncFinished(bool* ok);
void syncCancel();

// Webserver bis Timeout oder keepRunning() == false. statusHtml liefert die Startseite.
void serveExport(const std::function<String()>& statusHtml,
                 const std::function<bool()>& keepRunning,
                 const std::function<void(const char* ip)>& onReady);

}  // namespace net

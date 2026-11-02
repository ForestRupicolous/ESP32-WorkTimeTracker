#!/usr/bin/env python3
"""Zeitprisma-Auswertung: log.csv -> Stunden je Projekt und Tag/Woche.

    python3 auswertung.py zeitprisma.csv              # Tage
    python3 auswertung.py zeitprisma.csv --woche      # ISO-Wochen
    python3 auswertung.py zeitprisma.csv --csv out.csv
    python3 auswertung.py zeitprisma.csv --ohne-datum   # Stunden aus Zeilen ohne Uhrzeit

Jede Zeile im Log ist ein Wechsel: ab 'zeit' gilt 'projekt'. Ein Abschnitt endet mit
dem naechsten Wechsel; der letzte offene Abschnitt endet "jetzt" (--bis ueberschreibt).
Zeilen ohne gueltige Uhrzeit (zeit_ok=0) werden in der Tages-/Wochenauswertung uebersprungen. Pause
(Projekt 0) wird nicht summiert. Abschnitte ueber Mitternacht werden geteilt.

Mit --ohne-datum werden nur die Zeilen ohne Uhrzeit ausgewertet (epoch = Sekunden seit dem Start des Sticks, kein
Datum): Stunden je Projekt ohne Tage. Eine Sitzung beginnt mit einer Zeile seite=-2 (Neustart) und endet mit seite=-3
(Uhrzeit gefunden) oder dem naechsten Neustart; ein offener letzter Abschnitt ohne Ende zaehlt nicht (sein Ende ist
unbekannt).
"""
import argparse
import csv
import sys
from collections import defaultdict
from datetime import datetime, timedelta


def read_events(path):
    ev = []
    with open(path, newline='', encoding='utf-8') as f:
        for r in csv.DictReader(f):
            if r.get('zeit_ok') != '1' or not r.get('zeit'):
                continue
            t = datetime.strptime(r['zeit'], '%Y-%m-%dT%H:%M:%S%z')
            ev.append((t, int(r['projekt']), r['name']))
    ev.sort(key=lambda e: e[0])
    return ev


SEITE_NEUSTART = -2     # Log ohne Uhrzeit: neue Zeitbasis
SEITE_ZEIT_GEFUNDEN = -3  # Log ohne Uhrzeit: Uhrzeit wieder gueltig, der Abschnitt davor endet hier


def read_nodate(path):
    """Zeilen ohne Uhrzeit (zeit_ok=0) in Dateireihenfolge: (epoch, seite, projekt, name)."""
    rows = []
    with open(path, newline='', encoding='utf-8') as f:
        for r in csv.DictReader(f):
            if r.get('zeit_ok') != '0':
                continue
            rows.append((int(r['epoch']), int(r['seite']), int(r['projekt']), r['name']))
    return rows


def summarize_nodate(rows):
    """Sekunden je Projektname und Anzahl der Sitzungen. Pause (Projekt 0) wird nicht summiert."""
    sums = defaultdict(float)
    sessions = 0
    prev = None  # (epoch, projekt, name) des laufenden Abschnitts
    for t, seite, proj, name in rows:
        if seite == SEITE_NEUSTART:
            prev = None
            sessions += 1
            continue
        if prev is not None and t >= prev[0] and prev[1] != 0:
            sums[prev[2]] += t - prev[0]
        # t < prev[0]: die Zeit sprang zurueck (Neustart ohne Markierung), der Abschnitt davor bleibt offen
        prev = None if seite == SEITE_ZEIT_GEFUNDEN else (t, proj, name)
    return sums, sessions


def split_days(start, end):
    """[start, end) an lokaler Mitternacht (Zeitzone von start) teilen."""
    while start < end:
        nxt = (start + timedelta(days=1)).replace(hour=0, minute=0, second=0)
        seg_end = min(end, nxt)
        yield start.date(), (seg_end - start).total_seconds()
        start = seg_end


def summarize(events, until, weekly=False):
    sums = defaultdict(float)  # (periode, name) -> Sekunden
    for i, (t, proj, name) in enumerate(events):
        end = events[i + 1][0] if i + 1 < len(events) else until
        if proj == 0 or end <= t:
            continue
        for day, sec in split_days(t, end):
            key = f"{day.isocalendar()[0]}-KW{day.isocalendar()[1]:02d}" if weekly else day.isoformat()
            sums[(key, name)] += sec
    return sums


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log')
    ap.add_argument('--woche', action='store_true')
    ap.add_argument('--bis', help='Ende des letzten Abschnitts, ISO-Zeit (Default: jetzt)')
    ap.add_argument('--csv', help='Ergebnis zusaetzlich als CSV schreiben')
    ap.add_argument('--ohne-datum', action='store_true', help='Zeilen ohne Uhrzeit auswerten (Stunden ohne Datum)')
    a = ap.parse_args()

    if a.ohne_datum:
        sums, sessions = summarize_nodate(read_nodate(a.log))
        if not sums:
            sys.exit('keine abgeschlossenen Abschnitte ohne Datum')
        w = max(10, *(len(n) for n in sums))
        print(f"{'Ohne Datum':{w}}{'Stunden':>10}   ({sessions} Sitzung(en) mit Neustart-Zeile)")
        for n in sorted(sums):
            print(f"{n:{w}}{sums[n] / 3600:>10.2f}")
        print(f"{'Summe':{w}}{sum(sums.values()) / 3600:>10.2f}")
        return

    events = read_events(a.log)
    if not events:
        sys.exit('keine gueltigen Eintraege')
    tz = events[-1][0].tzinfo
    until = datetime.fromisoformat(a.bis) if a.bis else datetime.now(tz)
    if until.tzinfo is None:
        until = until.replace(tzinfo=tz)
    sums = summarize(events, until, a.woche)
    skipped = len(read_nodate(a.log))
    if skipped:
        print(f"Hinweis: {skipped} Zeilen ohne Uhrzeit uebersprungen (--ohne-datum wertet sie aus)", file=sys.stderr)

    periods = sorted({p for p, _ in sums})
    names = sorted({n for _, n in sums})
    w = max(10, *(len(n) for n in names))
    print(f"{'':12}" + ''.join(f"{n:>{w}}" for n in names) + f"{'Summe':>{w}}")
    for p in periods:
        row = [sums.get((p, n), 0) / 3600 for n in names]
        print(f"{p:12}" + ''.join(f"{h:>{w}.2f}" for h in row) + f"{sum(row):>{w}.2f}")
    if a.csv:
        with open(a.csv, 'w', newline='', encoding='utf-8') as f:
            wr = csv.writer(f)
            wr.writerow(['periode', 'projekt', 'stunden'])
            for (p, n), s in sorted(sums.items()):
                wr.writerow([p, n, f"{s / 3600:.2f}"])


if __name__ == '__main__':
    main()

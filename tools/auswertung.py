#!/usr/bin/env python3
"""Zeitprisma-Auswertung: log.csv -> Stunden je Projekt und Tag/Woche.

    python3 auswertung.py zeitprisma.csv              # Tage
    python3 auswertung.py zeitprisma.csv --woche      # ISO-Wochen
    python3 auswertung.py zeitprisma.csv --csv out.csv

Jede Zeile im Log ist ein Wechsel: ab 'zeit' gilt 'projekt'. Ein Abschnitt endet mit
dem naechsten Wechsel; der letzte offene Abschnitt endet "jetzt" (--bis ueberschreibt).
Zeilen ohne gueltige Uhrzeit (zeit_ok=0) werden uebersprungen. Pause (Projekt 0)
wird nicht summiert. Abschnitte ueber Mitternacht werden geteilt.
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
    a = ap.parse_args()

    events = read_events(a.log)
    if not events:
        sys.exit('keine gueltigen Eintraege')
    tz = events[-1][0].tzinfo
    until = datetime.fromisoformat(a.bis) if a.bis else datetime.now(tz)
    if until.tzinfo is None:
        until = until.replace(tzinfo=tz)
    sums = summarize(events, until, a.woche)

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

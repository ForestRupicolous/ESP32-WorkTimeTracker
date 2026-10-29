#!/usr/bin/env python3
"""Serielle Ausgabe des StickS3 nicht-interaktiv mitschreiben (fuer Claude Code / Skripte).

    python3 tools/seriallog.py                    # 30 s, Port automatisch (Espressif VID 303A)
    python3 tools/seriallog.py -s 120 -o log.txt  # 120 s, zusaetzlich in Datei
    python3 tools/seriallog.py -p /dev/ttyACM0

Der StickS3 nutzt das native USB des ESP32-S3: im Deep Sleep verschwindet der Port und
taucht beim Aufwachen wieder auf. Das Skript verbindet dann automatisch neu und markiert
die Luecke im Log.
"""
import argparse
import sys
import time

import serial                      # pyserial (kommt mit PlatformIO mit)
from serial.tools import list_ports

ESPRESSIF_VID = 0x303A


def find_port():
    for p in list_ports.comports():
        if p.vid == ESPRESSIF_VID:
            return p.device
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-p', '--port', help='z. B. /dev/ttyACM0 (Standard: automatisch)')
    ap.add_argument('-s', '--sekunden', type=float, default=30.0)
    ap.add_argument('-o', '--datei', help='zusaetzlich in Datei schreiben')
    ap.add_argument('-b', '--baud', type=int, default=115200)
    a = ap.parse_args()

    out = open(a.datei, 'a', encoding='utf-8') if a.datei else None
    t_end = time.monotonic() + a.sekunden
    t0 = time.monotonic()
    ser, was_connected = None, False

    def emit(line):
        s = f"[{time.monotonic() - t0:7.2f}] {line}"
        print(s, flush=True)
        if out:
            out.write(s + '\n'); out.flush()

    while time.monotonic() < t_end:
        if ser is None:
            port = a.port or find_port()
            if port:
                try:
                    ser = serial.Serial(port, a.baud, timeout=0.2)
                    emit(f"--- verbunden mit {port}")
                    was_connected = True
                except (serial.SerialException, OSError) as e:
                    if 'ermission' in str(e):
                        emit(f"--- keine Berechtigung fuer {port}: Benutzer in Gruppe 'dialout'?")
                        return 2
                    ser = None
            if ser is None:
                time.sleep(0.3)
                continue
        try:
            raw = ser.readline()
        except (serial.SerialException, OSError):
            emit("--- Port weg (Deep Sleep oder Reset)")
            ser = None
            continue
        if raw:
            emit(raw.decode('utf-8', errors='replace').rstrip())
    if ser:
        ser.close()
    if not was_connected:
        emit("--- kein StickS3 gefunden (USB-Kabel? Geraet wach?)")
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())

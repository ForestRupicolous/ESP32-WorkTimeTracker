#!/usr/bin/env bash
# Stick fangen -> flashen (make flash) -> per Watchdog-Reset starten -> Log mitschneiden.
#
# Aufruf: tools/flash_und_log.sh [-n] [Mitschnitt-Sekunden, Standard 75] [Fang-Zeitlimit, Standard 240]
#   -n   Teach-in neu ausloesen: nach dem Flashen die NVS-Partition (0x9000, 0x5000) sichern
#        und loeschen, das Seitenmodell wird beim ersten Start neu eingelernt.
# Umgebung: ENV (Standard muenze8), OUT (Ablage fuer Log und Sicherung,
#   Standard $TMPDIR/zeitprisma_flash), PYTHON, ESPTOOL.
#
# Ablauf am Stick: Skript starten, dann den Stick bewegen (schlafender Stick hat keinen
# USB-Port, siehe stick_fangen.sh). Ein Watchdog-Reset startet die App nach dem Flashen,
# der RTS-Hard-Reset von esptool fuehrt hier zurueck in den Bootloader.
# WLAN-Namen (config_local.h) werden aus dem Mitschnitt entfernt, damit sie nicht im Terminal stehen.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/.." && pwd)"
PY="${PYTHON:-$HOME/.platformio/penv/bin/python}"
ES="${ESPTOOL:-$HOME/.platformio/packages/tool-esptoolpy/esptool.py}"
OUT="${OUT:-${TMPDIR:-/tmp}/zeitprisma_flash}"
FWENV="${ENV:-muenze8}"
NVS_OFF=0x9000
NVS_LEN=0x5000

TEACH=0
if [ "$1" = "-n" ]; then TEACH=1; shift; fi
SECS="${1:-75}"
mkdir -p "$OUT" || exit 3

echo "== Fangen ($(date +%T))"
bash "$HERE/stick_fangen.sh" "${2:-240}" || { echo "KEIN Bootloader erreicht"; exit 1; }

echo "== Flashen ($FWENV)"
cd "$R" || exit 3
PATH="$HOME/.platformio/penv/bin:$PATH" make flash ENV="$FWENV" > "$OUT/pio_flash.log" 2>&1
rc=$?
sed 's/\x1b\[[0-9;]*m//g' "$OUT/pio_flash.log" | grep -E "Wrote|Hash of data|SUCCESS|FAILED|rror" | head -10
[ "$rc" -eq 0 ] || { echo "FLASH FEHLGESCHLAGEN (rc=$rc)"; exit 2; }

RESET_AFTER=watchdog_reset
if [ "$TEACH" -eq 1 ]; then
  echo "== NVS sichern ($NVS_OFF, $NVS_LEN)"
  B="$OUT/nvs_backup_$(date +%H%M%S).bin"
  P=$(ls /dev/ttyACM* 2>/dev/null | tail -1)
  [ -n "$P" ] || { echo "KEIN Port nach dem Flashen"; exit 4; }
  timeout 60 "$PY" "$ES" --chip esp32s3 --port "$P" --before default_reset --after no_reset \
    read_flash "$NVS_OFF" "$NVS_LEN" "$B" 2>&1 | grep -E "Read|rror|Hash" | head -5
  [ -s "$B" ] || { echo "SICHERUNG FEHLGESCHLAGEN, NVS wird nicht geloescht"; exit 5; }
  echo "Sicherung: $B ($(stat -c %s "$B") Bytes)"
  P=$(ls /dev/ttyACM* 2>/dev/null | tail -1)
  [ -n "$P" ] || { echo "KEIN Port fuer das Loeschen"; exit 6; }
  echo "== NVS loeschen und per Watchdog-Reset starten, Mitschnitt $SECS s ($(date +%T))"
  timeout 60 "$PY" "$ES" --chip esp32s3 --port "$P" --before default_reset --after "$RESET_AFTER" \
    erase_region "$NVS_OFF" "$NVS_LEN" 2>&1 | grep -E "Erase|rror" | head -5
else
  echo "== Watchdog-Reset und Mitschnitt $SECS s ($(date +%T))"
  P=$(ls /dev/ttyACM* 2>/dev/null | tail -1)
  [ -n "$P" ] && timeout 40 "$PY" "$ES" --chip esp32s3 --port "$P" --before default_reset \
    --after "$RESET_AFTER" chip_id >/dev/null 2>&1
fi

# Netznamen aus der Ausgabe filtern (alle drei Eintraege, falls gesetzt)
PAT="$(mktemp)"; chmod 600 "$PAT"
sed -n 's/^#define WIFI_SSID\(_[23]\)\?[[:space:]]*"\(.\+\)"[[:space:]]*$/\2/p' \
  firmware/include/config_local.h 2>/dev/null > "$PAT"
"$PY" -I tools/seriallog.py -s "$SECS" -o "$OUT/last_log.txt" 2>&1 \
  | grep -vFf "$PAT" | grep -vE "i2cInit|I2S"
rm -f "$PAT"
echo "== Ende ($(date +%T)), Log: $OUT/last_log.txt"

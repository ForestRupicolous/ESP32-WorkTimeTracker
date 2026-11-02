#!/usr/bin/env bash
# Faengt den schlafenden StickS3 in einem Wach-Fenster und setzt ihn in den ROM-Bootloader
# (bleibt dort). Im Deep Sleep hat der Stick keinen USB-Port: Waehrend das Skript laeuft,
# den Stick bewegen oder Taste A druecken, dann verbindet es im ersten Wachfenster.
#
# Aufruf: tools/stick_fangen.sh [Zeitlimit in Sekunden, Standard 240]
# Voraussetzung: PlatformIO mit esptool (~/.platformio, oder ESPTOOL / PYTHON setzen).
PY="${PYTHON:-$HOME/.platformio/penv/bin/python}"
ES="${ESPTOOL:-$HOME/.platformio/packages/tool-esptoolpy/esptool.py}"
end=$((SECONDS + ${1:-240}))
n=0
while [ "$SECONDS" -lt "$end" ]; do
  P=$(ls /dev/ttyACM* 2>/dev/null | head -1)
  if [ -n "$P" ]; then
    n=$((n + 1))
    if timeout 15 "$PY" "$ES" --chip esp32s3 --port "$P" --before default_reset --after no_reset \
         --connect-attempts 1 flash_id >/dev/null 2>&1; then
      echo "IM BOOTLOADER nach $n Versuchen (Port $P, $(date +%T))"
      exit 0
    fi
  fi
  sleep 0.05
done
echo "ZEITLIMIT nach $n Versuchen ($(date +%T))"
exit 1

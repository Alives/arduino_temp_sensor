#!/bin/bash
# Helper script to compile the sketch using arduino-cli with all warning flags and CPU cores.

# FQBN for NodeMCUv2 with 1MB LittleFS layout and 115200 baud upload speed.
BOARD_FQBN="esp8266:esp8266:nodemcuv2:eesz=4M1M,baud=115200"

echo "Compiling sketch..."
arduino-cli compile --fqbn "$BOARD_FQBN" --warnings all --jobs 0 --output-dir bin . "$@"

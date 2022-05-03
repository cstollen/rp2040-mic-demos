#! /bin/bash

SERIAL_PORT=/dev/ttyACM0
MICRO_PATH=/media/${USER}/RPI-RP2
MAGIC_BAUD_RATE=1200
BUILD_PATH=build

if [ ! -f "CMakeLists.txt" ] || [ ! -f "cmake/pico_sdk_import.cmake" ] || [ ! -d "src" ] || [ ! -d "lib" ]; then
  echo "Execute upload script from project root folder containing CMakeLists.txt"
  exit 1
fi

serial_reset=false
if [ -c "$SERIAL_PORT" ]; then
  stty -F $SERIAL_PORT $MAGIC_BAUD_RATE
  serial_reset=true
  echo "Reset device by changing baud rate on serial port $SERIAL_PORT to $MAGIC_BAUD_RATE"
fi

waited=false
if [ ! -d "$MICRO_PATH" ]; then
  if [[ "$serial_reset" = false ]]; then
    echo "Please reset microcontroller (Arduino RP2040 Nano: Bridge white pin 2 to pin 3 and press RESET button)"
  fi
  echo -n "Waiting for mounted microcontroller filesystem $MICRO_PATH "
  waited=true
fi
while [ ! -d "$MICRO_PATH" ]; do
  echo -n "."
  sleep 0.75
done

if [[ "$waited" = true ]]; then
  echo ""
fi

find $BUILD_PATH -type f -name "*.uf2" -print0 | while read -d $'\0' file
do
  cp $file $MICRO_PATH
  if [ $? -ne 0 ]; then
    echo "Could not upload $file to $MICRO_PATH"
  else
    echo "Uploaded $file to $MICRO_PATH"
  fi
  break
done


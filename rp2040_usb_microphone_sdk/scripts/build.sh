#! /bin/bash

if [ ! -f "CMakeLists.txt" ] || [ ! -f "cmake/pico_sdk_import.cmake" ] || [ ! -d "src" ] || [ ! -d "lib/microphone-library-for-pico" ]; then
  echo "Execute build script from project root folder containing CMakeLists.txt"
  exit 1
fi

if [ ! -d "build" ]; then
  mkdir build
fi
cd build
cmake ..
make -j$(nproc)


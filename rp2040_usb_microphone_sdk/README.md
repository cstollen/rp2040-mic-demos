# RP2040 USB Microphone
This project enables an Arduino Nano RP2040 Connect to be used as an USB microphone device.
It is based on the [USB microphone example][usb-mic-example] of the [microphone library for pico][microphone-library-for-pico].

## Dependencies
- [CMake][cmake] >= 3.12
- [Raspberry Pi Pico Arduino core for RP2040 boards][arduino-pico-sdk]
- [Microphone library for pico][microphone-library-for-pico]

### Installation of Arduino Pico SDK
Installing the Arduino Pico SDK to `~/arduino-pico`:  
`cd ~`  
`git clone --depth 1 https://github.com/earlephilhower/arduino-pico.git`  
`cd arduino-pico`  
`git submodule update --init`  
`cd pico-sdk`  
`git submodule update --init`  
`cd ../pico-extras`  
`git submodule update --init`  
`cd ../tools`  
`python3 ./get.py`  

Add to `.bashrc`:  
`export PICO_SDK_PATH=~/arduino-pico/pico-sdk`  
`export PICO_TOOLCHAIN_PATH=~/arduino-pico/system/arm-none-eabi/bin`  

## Build Project
Run build and upload scripts from `CMakeLists.txt` root directory to build project into directory `build`:

- Build: `./scripts/build.sh`  
- Build clean: `./scripts/build-clean.sh`  
- Upload: `./scripts/upload-uf2.sh`  
- Build and upload: `.scripts/build-and-upload.sh`  


[cmake]: https://cmake.org/
[arduino-pico-sdk]: https://github.com/earlephilhower/arduino-pico
[usb-mic-example]: https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main/examples/usb_microphone
[microphone-library-for-pico]: https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico


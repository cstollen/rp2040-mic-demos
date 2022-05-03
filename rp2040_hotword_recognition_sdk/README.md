# RP2040 Hotword Recognition
This project enables an Arduino Nano RP2040 Connect to recognize hotwords. The results are sent to the serial over USB interface.
It is based on the [USB microphone example][usb-mic-example] of the [microphone library for pico][microphone-library-for-pico] and the [Tensorflow micro speech example][tflm-speech-example].

## Dependencies
- [CMake][cmake] >= 3.12
- [Raspberry Pi Pico Arduino core for RP2040 boards][arduino-pico-sdk]
- [Microphone library for pico][microphone-library-for-pico] (in `lib` directory)
- [Tensorflow Lite Micro][tflm] (in `lib` directory)

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

## Serial Monitor
A simple serial monitor using `cat` can be run to get the output of the hotword recognition:  
`./scripts/serial-monitor.sh`  
The device has initialized, when the onboard LED lights up and it is waiting for a serial connection to resume.  
The output consists of the recognized word, a score and the time since the start of the device.  

## Default Device Paths
Note: It is assumed, that the Arduino will be mounted on  
`/media/${USER}/RPI-RP2`  
after reset and that the serial interface is available at:  
`/dev/ttyACM0`

## Changing Parameters
Important changeable parameters can be found in `src/config.h` including microphone and recognition configuration.

## Loading Test Data
To load testdata instead of using the microphone, uncomment `#define LOADDATA` in `src/audio_provider.cpp`.  
The example data consists of audio samples containing the words "yes" and "no".  
Custom data was recorded containing the words "yes" and "no" in a 4 second audio clip. The custom data can be loaded by also uncommenting `#define CUSTOMDATA`.

## Switching Tensorflow Models
Tensorflow Lite Micro models were trained using the [speech commands dataset][speech-commands-dataset] by [Pete Warden][speech-commands-dataset-paper] available as [download][speech-commands-dataset-download]. It contains 35 words, from which a subset is chosen as recognizable hotwords.  
Included in the project are 3 trained models for the word sets:  

- 2 words: "yes", "no"  
- 8 words: "yes", "no", "up", "down", "left", "right", "on", "off"  
- 10 words:  "yes", "no", "up", "down", "left", "right", "on", "off", "stop", "go"  

The model is chosen by setting the `WORDCOUNT` variable in `CMakeLists.txt` to 2, 8 or 10. Just the used model is compiled and uploaded to the Arduino.  
For the training a jupyter notebook script found in `scripts/training` was used.

### Model Performances
2 words (1236 test samples):  

- Float model:  
	- Size: 68044 bytes  
	- Accuracy: 92.071197%  
- Quantized model:  
	- Size: 18712 bytes  
	- Accuracy: 91.828479%  

8 words (3915 test samples):  

- Float model:  
	- Size: 164068 bytes  
	- Accuracy: 76.653895%  
- Quantized model:  
	- Size: 42736 bytes  
	- Accuracy: 76.704981%  

10 words (4726 test samples):  

- Float model:  
	- Size: 196076 bytes  
	- Accuracy: 72.598392%  
- Quantized model:  
	- Size: 50744 bytes  
	- Accuracy: 72.577232%  


[cmake]: https://cmake.org/
[arduino-pico-sdk]: https://github.com/earlephilhower/arduino-pico
[usb-mic-example]: https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main/examples/usb_microphone
[microphone-library-for-pico]: https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico
[tflm-speech-example]: https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech
[tflm]: https://github.com/tensorflow/tflite-micro
[speech-commands-dataset]: https://www.tensorflow.org/datasets/catalog/speech_commands
[speech-commands-dataset-paper]: https://arxiv.org/abs/1804.03209
[speech-commands-dataset-download]: http://download.tensorflow.org/data/speech_commands_v0.02.tar.gz



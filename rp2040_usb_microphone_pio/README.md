# RP2040 USB Microphone
This project enables an Arduino Nano RP2040 Connect to be used as an USB microphone device.
It is based on the [Arduino microphone example] and the [mbed USB Audio API].

## Dependencies
- [PlatformIO]  
- PlatformIO packages:  
	- framework-arduino-mbed 3.0.1   
	- tool-openocd-raspberrypi 2.1100.0 (11.0)   
	- tool-rp2040tools 1.0.2   
	- toolchain-gccarmnoneeabi 1.90301.200702 (9.3.1)  
- PlatformIO libraries:  
	- PDM 1.0  

### Installation of Platformio
Installing PlatformIO in a python 3 environment:  
`pip3 install platformio`  
Update udev rules:  
`sudo ~/.platformio/packages/framework-arduino-mbed/post_install.sh`  

## Build Project
- Build: `pio run`  
- Build and upload: `pio run -t upload`  


[PlatformIO]: https://platformio.org
[Arduino microphone example]: https://docs.arduino.cc/tutorials/nano-rp2040-connect/rp2040-microphone-basics
[mbed USB Audio API]: https://os.mbed.com/handbook/USBAudio

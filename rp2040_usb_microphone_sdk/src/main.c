/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This examples creates a USB Microphone device using the TinyUSB
 * library and captures data from a PDM microphone using a sample
 * rate of 16 kHz, to be sent the to PC.
 *
 * The USB microphone code is based on the TinyUSB audio_test example.
 *
 * https://github.com/hathach/tinyusb/tree/master/examples/device/audio_test
 */

#define SAMPLE_RATE 16000      // 16000 Hz
#define SAMPLE_BUFFER_SIZE 16  // 16 samples/ms * 1 channel

#include "pico/pdm_microphone.h"
#include "usb_microphone.h"

// Arduino Nano RP2040 Connect pin definitions
// https://github.com/arduino/ArduinoCore-mbed/blob/master/variants/NANO_RP2040_CONNECT/pins_arduino.h
// https://github.com/earlephilhower/arduino-pico/blob/master/variants/arduino_nano_connect/pins_arduino.h

// Configuration
const struct pdm_microphone_config config = {
    .gpio_data = 22,  // PIN_PDM_DIN 22
    .gpio_clk = 23,   // PIN_PDM_CLK 23
    .pio = pio0,
    .pio_sm = 0,
    .sample_rate = SAMPLE_RATE,
    .sample_buffer_size = SAMPLE_BUFFER_SIZE,
};

// Variables
uint16_t sample_buffer[SAMPLE_BUFFER_SIZE];

// Callback functions
void on_pdm_samples_ready();
void on_usb_microphone_tx_ready();

void init_pdm_microphone() {
	uint8_t mic_filter_gain = 16;        // default: 16
	uint8_t mic_filter_max_volume = 64;  // default: 64
	uint16_t mic_filter_volume = 64;     // default: 64
	float mic_filter_highpass_hz = 10;   // default: 10
	float mic_filter_lowpass_hz = 8000;  // default: sample_rate / 2

	// Initialize and start the PDM microphone
	pdm_microphone_init(&config);
	pdm_microphone_set_filter_gain(mic_filter_gain);
	pdm_microphone_set_filter_max_volume(mic_filter_max_volume);
	pdm_microphone_set_filter_volume(mic_filter_volume);
	pdm_microphone_set_filter_lowpass_hz(mic_filter_lowpass_hz);
	pdm_microphone_set_filter_highpass_hz(mic_filter_highpass_hz);
	pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
	pdm_microphone_start();
}

void init_usb_microphone() {
	// Initialize the USB microphone interface
	usb_microphone_init();
	usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);
}

int main(void) {
	// Initialize
	init_pdm_microphone();
	init_usb_microphone();

	while (1) {
		// Run the USB microphone task continuously
		usb_microphone_task();
	}

	return 0;
}

// Callback from library when all the samples in the library internal sample buffer are ready for reading.
void on_pdm_samples_ready() {
	// Read new samples into local buffer.
	pdm_microphone_read(sample_buffer, SAMPLE_BUFFER_SIZE);
}

// Callback from TinyUSB library when all data is ready to be transmitted.
void on_usb_microphone_tx_ready() {
	// Write local buffer to the USB microphone
	usb_microphone_write(sample_buffer, sizeof(sample_buffer));
}

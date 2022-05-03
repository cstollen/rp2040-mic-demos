#include <Arduino.h>
#include <PDM.h>
#include "USBAudio.h"

// Buffer size
const int sample_buffer_size = 64;
// default number of output channels
static const int channels = 1;
// default PCM output frequency
static const int frequency = 16000;
// Buffer to read samples into, each sample is 16-bits
short sample_buffer[sample_buffer_size];
// Number of audio samples read
volatile int samples_read = 0;
// Microphone gain
const int mic_gain = 16;  // default: 16

// USB audio device
USBAudio *audio;

void initPDM();
void initUSBAudio();
void onPDMdata();
void writeUSBAudio();

void setup() {
	// Setup builtin led
	pinMode(LED_BUILTIN, OUTPUT);

	// Init PDM microphone
	initPDM();

	// Init USB audio device
	initUSBAudio();

	// Light up builtin led on successful setup
	digitalWrite(LED_BUILTIN, HIGH);
}

void loop() { writeUSBAudio(); }

void initPDM() {
	// Configure the data receive callback
	PDM.onReceive(onPDMdata);

	// Set buffer size
	PDM.setBufferSize(sample_buffer_size * 2);

	// Set the microphone gain
	// Defaults to 20 on the BLE Sense and -10 on the Portenta Vision Shields
	// PDM library default: 16 (set when _gain is -1)
	PDM.setGain(mic_gain);

	// Initialize PDM with:
	// - one channel (mono mode)
	// - a 16 kHz sample rate for the Arduino Nano 33 BLE Sense
	// - a 32 kHz or 64 kHz sample rate for the Arduino Portenta Vision Shields
	if (!PDM.begin(channels, frequency)) {
		while (1)
			;
	}
}

void initUSBAudio() {
	int buffer_ms = 10;  // 10 ms = 160 samples at 16000 Hz
	audio = new USBAudio(true, frequency, 1, frequency, 1, buffer_ms, 0x7bb8, 0x1112, 0x0100);
}

/**
   Callback function to process the data from the PDM microphone.
   NOTE: This callback is executed as part of an ISR.
   Therefore using `Serial` to print messages inside this function isn't supported.
 **/
void onPDMdata() {
	// Query the number of available bytes
	int bytes_available = PDM.available();
	// Read into the sample buffer
	PDM.read(sample_buffer, bytes_available);
	// 16-bit, 2 bytes per sample
	samples_read = bytes_available / 2;
}

// Write mic sample buffer
void writeUSBAudio() {
	if (samples_read > 0) {
		if (!audio->write((uint8_t *)&sample_buffer, samples_read * 2)) {
			audio->write_wait_ready();
		}
		// Clear the read count
		samples_read = 0;
	}
}

/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// #define LOADDATA // Load data from files
// #define CUSTOMDATA // Load custom 4s yes/no audio clip (LOADDATA has to be defined)
// #define PRINTTIMINGS // Print timing info

// https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech

#include "config.h"
#include "audio_provider.h"
#include "micro_features/micro_model_settings.h"
#include <Arduino.h>

#ifndef LOADDATA

#include <PDM.h>

namespace {
bool g_is_audio_initialized = false;
// An internal buffer able to fit 16x our sample size
constexpr int kAudioCaptureBufferSize = DEFAULT_PDM_BUFFER_SIZE * 16;  // 512 * 16
int16_t g_audio_capture_buffer[kAudioCaptureBufferSize];
// A buffer that holds our output
int16_t g_audio_output_buffer[kMaxAudioSampleSize];
// Mark as volatile so we can check in a while loop to see if
// any samples have arrived yet.
volatile int32_t g_latest_audio_timestamp = 0;

// Recorded time to buffer
volatile int32_t g_recorded_time_ms = 0;
}  // namespace

#ifdef PRINTTIMINGS
// Timings
volatile int rec_timestamp = 0;
volatile int rec_time = 0;
volatile int rec_interval = 0;
int write_timestamp = 0;
int write_time = 0;
int write_interval = 0;
#endif

void CaptureSamples() {
#ifdef PRINTTIMINGS
	int start_time = micros();
#endif
	// This is how many bytes of new data we have each time this is called
	int number_of_bytes = PDM.available();
	int number_of_samples = number_of_bytes / 2;
	// Calculate what timestamp the last audio sample represents
	const int32_t time_in_ms =
	    g_latest_audio_timestamp + g_recorded_time_ms + (number_of_samples / (kAudioSampleFrequency / 1000));
	// Determine the index, in the history of all samples, of the last sample
	const int32_t start_sample_offset = (g_latest_audio_timestamp + g_recorded_time_ms) * (kAudioSampleFrequency / 1000);
	// Determine the index of this sample in our ring buffer
	const int capture_index = start_sample_offset % kAudioCaptureBufferSize;
	// Read the data to the correct place in our buffer
	PDM.read(g_audio_capture_buffer + capture_index, number_of_bytes);
	// This is how we let the outside world know that new audio data has arrived.
	// g_latest_audio_timestamp = time_in_ms;

	// Only report new data when recorded more than the feature slice duration
	const int record_report_time_ms = kFeatureSliceDurationMs;
	g_recorded_time_ms += (number_of_samples / (kAudioSampleFrequency / 1000));
	if (g_recorded_time_ms >= record_report_time_ms) {
		g_latest_audio_timestamp = time_in_ms;
		g_recorded_time_ms = 0;
	}

#ifdef PRINTTIMINGS
	int end_time = micros();
	rec_time = end_time - start_time;
	rec_interval = end_time - rec_timestamp;
	rec_timestamp = end_time;
#endif
}

TfLiteStatus InitAudioRecording(tflite::ErrorReporter* error_reporter) {
	// Hook up the callback that will be called with each sample
	PDM.onReceive(CaptureSamples);
	// Start listening for audio: MONO @ 16KHz
	int mic_channels = 1;
	int mic_frequency = kAudioSampleFrequency;  // 16000 Hz
	PDM.setBufferSize(DEFAULT_PDM_BUFFER_SIZE * 2);
	PDM.setGain(g_mic_filter_gain);
	if (!PDM.begin(mic_channels, mic_frequency)) {
		return kTfLiteError;
	};
	// Block until we have our first audio sample
	while (!g_latest_audio_timestamp) {
	}

	return kTfLiteOk;
}

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter, int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
#ifdef PRINTTIMINGS
	int start_time = micros();
#endif
	// Set everything up to start receiving audio
	if (!g_is_audio_initialized) {
		TfLiteStatus init_status = InitAudioRecording(error_reporter);
		if (init_status != kTfLiteOk) {
			return init_status;
		}
		g_is_audio_initialized = true;
	}
	// This next part should only be called when the main thread notices that the
	// latest audio sample data timestamp has changed, so that there's new data
	// in the capture ring buffer. The ring buffer will eventually wrap around and
	// overwrite the data, but the assumption is that the main thread is checking
	// often enough and the buffer is large enough that this call will be made
	// before that happens.

	// Determine the index, in the history of all samples, of the first
	// sample we want
	const int start_offset = start_ms * (kAudioSampleFrequency / 1000);
	// Determine how many samples we want in total
	const int duration_sample_count = duration_ms * (kAudioSampleFrequency / 1000);
	for (int i = 0; i < duration_sample_count; ++i) {
		// For each sample, transform its index in the history of all samples into
		// its index in g_audio_capture_buffer
		const int capture_index = (start_offset + i) % kAudioCaptureBufferSize;
		// Write the sample to the output buffer
		g_audio_output_buffer[i] = g_audio_capture_buffer[capture_index];
	}

	// Set pointers to provide access to the audio
	*audio_samples_size = kMaxAudioSampleSize;
	*audio_samples = g_audio_output_buffer;

#ifdef PRINTTIMINGS
	int end_time = micros();
	write_time = end_time - start_time;
	write_interval = end_time - write_timestamp;
	write_timestamp = end_time;
	Serial.print("rec time: ");
	Serial.println(rec_time);
	Serial.print("rec interval: ");
	Serial.println(rec_interval);
	Serial.print("write time: ");
	Serial.println(write_time);
	Serial.print("write interval: ");
	Serial.println(write_interval);
#endif

	return kTfLiteOk;
}

int32_t LatestAudioTimestamp() { return g_latest_audio_timestamp; }

#else  // LOADDATA
#ifndef CUSTOMDATA

// https://raw.githubusercontent.com/adafruit/Adafruit_TFLite_Micro_Speech/master/examples/micro_speech_mock/audio_provider.cpp
#include "testdata/no_1000ms_audio_data.h"
#include "testdata/yes_1000ms_audio_data.h"
#include "testdata/rec_1000ms_sample_data.h"

#include <Arduino.h>

namespace {
int16_t g_dummy_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter, int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
	digitalWrite(13, HIGH);
	const int yes_start = (0 * kAudioSampleFrequency) / 1000;
	const int yes_end = (1000 * kAudioSampleFrequency) / 1000;

	const int no_start = (2000 * kAudioSampleFrequency) / 1000;
	const int no_end = (3000 * kAudioSampleFrequency) / 1000;

	const int rec_start = (5000 * kAudioSampleFrequency) / 1000;
	const int rec_end = (6000 * kAudioSampleFrequency) / 1000;

	const int wraparound = (8000 * kAudioSampleFrequency) / 1000;
	const int start_sample = (start_ms * kAudioSampleFrequency) / 1000;
	for (int i = 0; i < kMaxAudioSampleSize; ++i) {
		const int sample_index = (start_sample + i) % wraparound;
		int16_t sample;
		if ((sample_index >= yes_start) && (sample_index < yes_end)) {
			sample = g_yes_1000ms_audio_data[sample_index - yes_start];
		} else if ((sample_index >= no_start) && (sample_index < no_end)) {
			sample = g_no_1000ms_audio_data[sample_index - no_start];
		} else if ((sample_index >= rec_start) && (sample_index < rec_end)) {
			sample = g_rec_1000ms_sample_data[sample_index - rec_start];
		} else {
			sample = 0;
		}
		g_dummy_audio_data[i] = sample;
	}
	*audio_samples_size = kMaxAudioSampleSize;
	*audio_samples = g_dummy_audio_data;
	digitalWrite(13, LOW);
	return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
	g_latest_audio_timestamp += 100;
	return g_latest_audio_timestamp;
}

#else  // CUSTOMDATA

#include "testdata/rec_yes_no_audio_data.h"

#include <Arduino.h>

namespace {
int16_t g_dummy_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter, int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
	digitalWrite(13, HIGH);

	const int wraparound = g_custom_audio_data_size;
	const int start_sample = (start_ms * kAudioSampleFrequency) / 1000;
	for (int i = 0; i < kMaxAudioSampleSize; ++i) {
		const int sample_index = (start_sample + i) % wraparound;
		int16_t sample;
		sample = g_custom_audio_data[sample_index];
		g_dummy_audio_data[i] = sample;
	}
	*audio_samples_size = kMaxAudioSampleSize;
	*audio_samples = g_dummy_audio_data;

	digitalWrite(13, LOW);
	return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
	g_latest_audio_timestamp += 100;
	return g_latest_audio_timestamp;
}

#endif  // CUSTOMDATA

#endif  // LOADDATA
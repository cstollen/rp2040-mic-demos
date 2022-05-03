/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

// https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech

// Project
#include "config.h"
#include "main_functions.h"
#include "audio_provider.h"
#include "command_responder.h"
#include "feature_provider.h"
#include "micro_features/micro_model_settings.h"
#include "micro_speech_model_data.h"
#include "recognize_commands.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
// Pico-sdk
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/pdm_microphone.h"
#include "tusb.h"
// Tensorflow logging
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* model_input = nullptr;
FeatureProvider* feature_provider = nullptr;
RecognizeCommands* recognizer = nullptr;
int32_t previous_time = 0;

// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 10 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
int8_t feature_buffer[kFeatureElementCount];
int8_t* model_input_buffer = nullptr;
}  // namespace

// Custom log function
void debug_log_printf(const char* s) { printf(s); }

// The name of this function is important for Arduino compatibility.
void setup() {
	// Initialize pico-sdk stdio
	stdio_init_all();
	// Init pico-sdk leds
	// raspberry pico PICO_DEFAULT_LED_PIN: 25
	// arduino rp2040 nano PICO_DEFAULT_LED_PIN: 6
	const uint LED_PIN = 6;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	// Custom log function
	RegisterDebugLogCallback(debug_log_printf);

	tflite::InitializeTarget();

	// Set up logging. Google style is to avoid globals or statics because of
	// lifetime uncertainty, but since this has a trivial destructor it's okay.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroErrorReporter micro_error_reporter;
	error_reporter = &micro_error_reporter;

	// Map the model into a usable data structure. This doesn't involve any
	// copying or parsing, it's a very lightweight operation.
	model = tflite::GetModel(g_micro_speech_model_data);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		TF_LITE_REPORT_ERROR(error_reporter,
		                     "Model provided is schema version %d not equal "
		                     "to supported version %d.",
		                     model->version(), TFLITE_SCHEMA_VERSION);
		return;
	}

	// Pull in only the operation implementations we need.
	// This relies on a complete list of all the ops needed by this graph.
	// An easier approach is to just use the AllOpsResolver, but this will
	// incur some penalty in code space for op implementations that are not
	// needed by this graph.
	//
	// tflite::AllOpsResolver resolver;
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroMutableOpResolver<4> micro_op_resolver(error_reporter);
	if (micro_op_resolver.AddDepthwiseConv2D() != kTfLiteOk) {
		return;
	}
	if (micro_op_resolver.AddFullyConnected() != kTfLiteOk) {
		return;
	}
	if (micro_op_resolver.AddSoftmax() != kTfLiteOk) {
		return;
	}
	if (micro_op_resolver.AddReshape() != kTfLiteOk) {
		return;
	}

	// Build an interpreter to run the model with.
	static tflite::MicroInterpreter static_interpreter(model, micro_op_resolver, tensor_arena, kTensorArenaSize,
	                                                   error_reporter);
	interpreter = &static_interpreter;

	// Allocate memory from the tensor_arena for the model's tensors.
	TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
		return;
	}

	// Get information about the memory area to use for the model's input.
	model_input = interpreter->input(0);
	if ((model_input->dims->size != 2) || (model_input->dims->data[0] != 1) ||
	    (model_input->dims->data[1] != (kFeatureSliceCount * kFeatureSliceSize)) || (model_input->type != kTfLiteInt8)) {
		TF_LITE_REPORT_ERROR(error_reporter, "Bad input tensor parameters in model");
		return;
	}
	model_input_buffer = model_input->data.int8;

	// Prepare to access the audio spectrograms from a microphone or other source
	// that will provide the inputs to the neural network.
	// NOLINTNEXTLINE(runtime-global-variables)
	static FeatureProvider static_feature_provider(kFeatureElementCount, feature_buffer);
	feature_provider = &static_feature_provider;

	static RecognizeCommands static_recognizer(error_reporter, g_rec_average_window_duration_ms,
	                                           g_rec_detection_threshold, g_rec_suppression_ms, g_rec_minimum_count);
	recognizer = &static_recognizer;

	previous_time = 0;

	// Wait for USB CDC serial connection
	gpio_put(LED_PIN, 1);
	while (!tud_cdc_connected()) {
		tight_loop_contents();
	}
	gpio_put(LED_PIN, 0);
	printf("Hotword recognition started\n");
}

// The name of this function is important for Arduino compatibility.
void loop() {
	// Fetch the spectrogram for the current time.
	const int32_t current_time = LatestAudioTimestamp();
	int how_many_new_slices = 0;
	TfLiteStatus feature_status =
	    feature_provider->PopulateFeatureData(error_reporter, previous_time, current_time, &how_many_new_slices);
	if (feature_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "Feature generation failed");
		return;
	}
	previous_time = current_time;
	// If no new audio samples have been received since last time, don't bother
	// running the network model.
	if (how_many_new_slices == 0) {
		return;
	}

	// Copy feature buffer to input tensor
	for (int i = 0; i < kFeatureElementCount; i++) {
		model_input_buffer[i] = feature_buffer[i];
	}

	// Run the model on the spectrogram input and make sure it succeeds.
	TfLiteStatus invoke_status = interpreter->Invoke();
	if (invoke_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed");
		return;
	}

	// Obtain a pointer to the output tensor
	TfLiteTensor* output = interpreter->output(0);
	// Determine whether a command was recognized based on the output of inference
	const char* found_command = nullptr;
	uint8_t score = 0;
	bool is_new_command = false;
	TfLiteStatus process_status =
	    recognizer->ProcessLatestResults(output, current_time, &found_command, &score, &is_new_command);
	if (process_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "RecognizeCommands::ProcessLatestResults() failed");
		return;
	}
	// Do something based on the recognized command. The default implementation
	// just prints to the error console, but you should replace this with your
	// own function for a real application.
	RespondToCommand(error_reporter, current_time, found_command, score, is_new_command);
}

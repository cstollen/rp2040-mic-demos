#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

// Microphone PDM filter parameters
const int g_mic_filter_gain = 16;  // default: 16

// Filter parameters are used during the PDM to PCM conversion.
// The filter gain is a factor used for amplification.

// Recognizer parameters
const int32_t g_rec_average_window_duration_ms = 500;  // default: 1000
const uint8_t g_rec_detection_threshold = 150;         // default: 200
const int32_t g_rec_suppression_ms = 1000;             // default: 1500
const int32_t g_rec_minimum_count = 3;                 // default: 3

// The window duration controls the smoothing. Longer durations will give a
// higher confidence that the results are correct, but may miss some commands.
// The detection threshold has a similar effect, with high values increasing
// the precision at the cost of recall.The minimum count controls how many
// results need to be in the averaging window before it's seen as a reliable
// average. This prevents erroneous results when the averaging window is
// initially being populated for example. The suppression argument disables
// further recognitions for a set time after one has been triggered, which can
// help reduce spurious recognitions.

#endif
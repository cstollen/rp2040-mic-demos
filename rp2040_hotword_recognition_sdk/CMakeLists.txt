cmake_minimum_required(VERSION 3.12)

# Set number of recognized words
# 2 -> "yes","no"
# 8 -> "yes","no","up","down","left","right","on","off"
# 10 -> "yes","no","up","down","left","right","on","off","stop","go"
set(WORDCOUNT 8)

# Project is based on pico-sdk environment variables PICO_SDK_PATH and PICO_TOOLCHAIN_PATH
# Defaults to using pico-sdk from https://github.com/earlephilhower/arduino-pico installed in /opt/arduino-pico
if(NOT DEFINED ENV{PICO_SDK_PATH})
  set (ENV{PICO_SDK_PATH} "/opt/arduino-pico/pico-sdk")
endif()
if(NOT DEFINED ENV{PICO_TOOLCHAIN_PATH})
  set(ENV{PICO_TOOLCHAIN_PATH} "/opt/arduino-pico/system/arm-none-eabi/bin")
endif()

# Optionally initialize pico_sdk from GIT
# set(PICO_SDK_FETCH_FROM_GIT on)

# Set microcontroller type
set(PICO_PLATFORM "rp2040")

# Set microcontroller board: Either "pico" or "arduino_nano_rp2040_connect"
set(PICO_BOARD "arduino_nano_rp2040_connect")

# pico_sdk_import.cmake is a cmake file copied from the pico SDK
# The pico-sdk include must happen before project()
include(cmake/pico_sdk_import.cmake)

# Project
set(PROJECT_NAME rp2040_hotword_recognition)
set(PROJECT_BINARY ${PROJECT_NAME})
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# Initialize the Pico SDK
pico_sdk_init()


#### Tensorflow Lite Mirco library
set(TFLM_LIBRARY tflm_lib)
set(TFLM_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/tflm)

file(GLOB_RECURSE TFLM_SOURCE_FILES ${TFLM_LIB_DIR}/tensorflow/lite/*.c ${TFLM_LIB_DIR}/tensorflow/lite/*.cpp ${TFLM_LIB_DIR}/third_party/*.c ${TFLM_LIB_DIR}/third_party/*.cpp)
file(GLOB_RECURSE TFLM_HEADER_FILES ${TFLM_LIB_DIR}/tensorflow/lite/*.h ${TFLM_LIB_DIR}/third_party/*.h)

add_library(${TFLM_LIBRARY} "")

target_include_directories(${TFLM_LIBRARY}
  PUBLIC
  ${TFLM_LIB_DIR}
  ${TFLM_LIB_DIR}/third_party
  ${TFLM_LIB_DIR}/third_party/flatbuffers/include
  ${TFLM_LIB_DIR}/third_party/gemmlowp
  ${TFLM_LIB_DIR}/third_party/kissfft/
  ${TFLM_LIB_DIR}/third_party/kissfft/tools
  ${TFLM_LIB_DIR}/third_party/ruy
  ${TFLM_LIB_DIR}/third_party/cmsis
  ${TFLM_LIB_DIR}/third_party/cmsis/CMSIS/NN/Include
  ${TFLM_LIB_DIR}/third_party/cmsis/CMSIS/DSP/Include
  ${TFLM_LIB_DIR}/third_party/cmsis/CMSIS/Core/Include
)

target_compile_definitions(
  ${TFLM_LIBRARY}
  PUBLIC
  COMPILE_DEFINITIONS TF_LITE_DISABLE_X86_NEON=1
  COMPILE_DEFINITIONS TF_LITE_STATIC_MEMORY=1
  COMPILE_DEFINITIONS CMSIS_NN=1
  # COMPILE_DEFINITIONS ARDUINOSTL_M_H=1 # for mbed core
)

target_link_libraries(
  ${TFLM_LIBRARY}
  pico_stdlib
)

target_sources(${TFLM_LIBRARY}
  PUBLIC
  ${TFLM_SOURCE_FILES}
  ${TFLM_HEADER_FILES}
)


#### PDM microphone library
set(MIC_LIBRARY mic_lib)
set(MIC_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/microphone-library-for-pico)

add_library(${MIC_LIBRARY} INTERFACE)

target_sources(${MIC_LIBRARY} INTERFACE
    ${MIC_LIB_DIR}/src/pdm_microphone.c
    ${MIC_LIB_DIR}/src/OpenPDM2PCM/OpenPDMFilter.c
)

target_include_directories(${MIC_LIBRARY} INTERFACE
    ${MIC_LIB_DIR}/src/include
)

pico_generate_pio_header(${MIC_LIBRARY} ${MIC_LIB_DIR}/src/pdm_microphone.pio)

target_link_libraries(${MIC_LIBRARY} INTERFACE pico_stdlib hardware_dma hardware_pio)


#### Project sources
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE PROJECT_SOURCE_FILES ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE PROJECT_HEADER_FILES ${SRC_DIR}/*.h)

# Add used model data file
list(FILTER PROJECT_SOURCE_FILES EXCLUDE REGEX "${SRC_DIR}/micro_speech_model_data.*\.cpp")
if(WORDCOUNT EQUAL 2)
  list(APPEND PROJECT_SOURCE_FILES ${SRC_DIR}/micro_speech_model_data_yesno.cpp)
elseif(WORDCOUNT EQUAL 8)
  list(APPEND PROJECT_SOURCE_FILES ${SRC_DIR}/micro_speech_model_data_8hotwords.cpp)
elseif(WORDCOUNT EQUAL 10)
  list(APPEND PROJECT_SOURCE_FILES ${SRC_DIR}/micro_speech_model_data_10hotwords.cpp)
else()
  message(FATAL_ERROR "WORDCOUNT must be 2, 8 or 10" )
endif()
add_compile_definitions(WORDCOUNT=${WORDCOUNT})

add_executable(${PROJECT_BINARY}
    ${PROJECT_SOURCE_FILES}
    ${PROJECT_HEADER_FILES}
)

target_include_directories(${PROJECT_BINARY} PRIVATE ${SRC_DIR})

set(PICO_SDK_LIBS pico_stdlib pico_time)
target_link_libraries(${PROJECT_BINARY} PRIVATE ${PICO_SDK_LIBS} ${TFLM_LIBRARY} ${MIC_LIBRARY})

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_BINARY} 1)
pico_enable_stdio_uart(${PROJECT_BINARY} 0)

# Create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_BINARY})

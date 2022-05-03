cmake_minimum_required(VERSION 3.12)

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
set(PROJECT_NAME rp2040_usb_microphone)
set(PROJECT_BINARY ${PROJECT_NAME})
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

# Initialize the Pico SDK
pico_sdk_init()

message(STATUS "PICO_BUILD = " ${PICO_BUILD})
message(STATUS "PICO_ON_DEVICE = " ${PICO_ON_DEVICE})
message(STATUS "PICO_BOARD = " ${PICO_BOARD})

#### PDM microphone library
set(MIC_LIBRARY pico_mic_lib)
set(MIC_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/microphone-library-for-pico)

set(MIC_LIB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/microphone-library-for-pico)

add_library(${MIC_LIBRARY} INTERFACE)

target_sources(${MIC_LIBRARY} INTERFACE
    ${MIC_LIB_PATH}/src/pdm_microphone.c
    ${MIC_LIB_PATH}/src/OpenPDM2PCM/OpenPDMFilter.c
)

target_include_directories(${MIC_LIBRARY} INTERFACE
    ${MIC_LIB_PATH}/src/include
)

pico_generate_pio_header(${MIC_LIBRARY} ${MIC_LIB_PATH}/src/pdm_microphone.pio)

target_link_libraries(${MIC_LIBRARY} INTERFACE pico_stdlib hardware_dma hardware_pio)


#### Configure project
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(${PROJECT_BINARY}
    ${SRC_DIR}/main.c
    ${SRC_DIR}/usb_descriptors.c
    ${SRC_DIR}/usb_microphone.c
)

target_include_directories(${PROJECT_BINARY} PRIVATE ${SRC_DIR})

target_link_libraries(${PROJECT_BINARY} PRIVATE tinyusb_device tinyusb_board ${MIC_LIBRARY})

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_BINARY})

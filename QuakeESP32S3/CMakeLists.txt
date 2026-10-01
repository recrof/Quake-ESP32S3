# Quake for ESP32-S3 (based on the MG24Quake port by Nicola Wrachien)
cmake_minimum_required(VERSION 3.16)
# Board selected on the command line (idf.py -B build-<board> -DBOARD=...): keep its
# configuration in the build directory, and apply its sdkconfig.defaults.<board> if any.
if(DEFINED BOARD)
    set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
    if(BOARD STREQUAL "BOARD_THINKNODE_M9")
        set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.defaults.m9")
    endif()
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(QuakeESP32S3)

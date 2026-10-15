# ESP-IDF Root CMakeLists.txt
# Haffler Single-Head UPVC Welding PLC/HMI System
cmake_minimum_required(VERSION 3.16)

# Register project-level components directory
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/components"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Include ESP-IDF project definitions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(welding_single_head)

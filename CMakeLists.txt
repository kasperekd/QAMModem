cmake_minimum_required(VERSION 3.20)
project(QAMModem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SIMD "Enable SIMD optimizations" OFF)
option(BUILD_TESTS "Build unit tests" OFF)
option(USE_PMR "Use std::pmr for memory allocation" ON)

# Include
include_directories(include)

# src
file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_library(modulator src/modulator/qam_modulator.cpp)
add_library(noise src/noise/noise_adder.cpp)
add_library(demodulator src/demodulator/qam_demodulator.cpp)
add_library(pipeline src/pipeline/simulator.cpp)

add_executable(qam_sim src/cli/main.cpp)
target_link_libraries(qam_sim PRIVATE modulator noise demodulator pipeline)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

if(USE_PMR)
    target_compile_definitions(pipeline PRIVATE USE_PMR)
endif()

if(ENABLE_SIMD)
    target_compile_definitions(noise PRIVATE ENABLE_SIMD)
    target_compile_options(noise PRIVATE -mavx2 -mfma)
endif()

add_custom_target(plot
    COMMAND python3 scripts/plot_ber.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
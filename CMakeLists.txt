cmake_minimum_required(VERSION 3.20)
project(QAMSimulator LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SIMD "Enable SIMD optimizations (AVX2/FMA)" ON)
option(BUILD_TESTS "Build unit tests with GoogleTest" OFF)
option(USE_PMR "Use std::pmr for memory management" ON)
option(BUILD_APPLICATION "Build main application" ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# SIMD
if(ENABLE_SIMD)
    add_compile_options(-mavx2 -mfma)
endif()

if(USE_PMR)
    add_compile_options(-DUSE_PMR)
endif()

# Корневые пути
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(INCLUDE_DIR ${PROJECT_ROOT}/include)
set(SRC_DIR ${PROJECT_ROOT}/src)
set(TESTS_DIR ${PROJECT_ROOT}/tests)
set(SCRIPTS_DIR ${PROJECT_ROOT}/scripts)

include_directories(${INCLUDE_DIR})
include_directories(${PROJECT_ROOT})

add_library(Modulator INTERFACE)
target_include_directories(Modulator INTERFACE ${INCLUDE_DIR})

add_library(Demodulator INTERFACE)
target_include_directories(Demodulator INTERFACE ${INCLUDE_DIR})

add_library(NoiseAdder INTERFACE)
target_include_directories(NoiseAdder INTERFACE ${INCLUDE_DIR})

if(ENABLE_SIMD)
    target_compile_definitions(NoiseAdder INTERFACE ENABLE_SIMD)
    target_compile_options(NoiseAdder INTERFACE -mavx2 -mfma)
endif()

if(USE_PMR)
    target_compile_definitions(Modulator INTERFACE USE_PMR)
    target_compile_definitions(Demodulator INTERFACE USE_PMR)
    target_compile_definitions(NoiseAdder INTERFACE USE_PMR)
endif()

if(BUILD_APPLICATION)
    add_library(Utils STATIC
        ${SRC_DIR}/utils/csv_writer.cpp
        ${SRC_DIR}/utils/random.cpp
    )
    target_include_directories(Utils PUBLIC ${INCLUDE_DIR})
    if(USE_PMR)
        target_compile_definitions(Utils PRIVATE USE_PMR)
    endif()
endif()

if(BUILD_APPLICATION)
    add_library(Pipeline STATIC
        ${SRC_DIR}/pipeline/qam_simulator.cpp
    )
    target_include_directories(Pipeline PUBLIC ${INCLUDE_DIR})
    target_link_libraries(Pipeline PRIVATE Modulator Demodulator NoiseAdder Utils)
endif()

if(BUILD_APPLICATION)
    add_executable(qam_simulator ${PROJECT_ROOT}/main.cpp)
    target_link_libraries(qam_simulator PRIVATE Pipeline)
    if(USE_PMR)
        target_compile_definitions(qam_simulator PRIVATE USE_PMR)
    endif()
endif()

if(BUILD_TESTS)
    find_package(GTest REQUIRED)

    file(GLOB TEST_SOURCES "${TESTS_DIR}/*.cpp")
    add_executable(qam_tests ${TEST_SOURCES})
    target_link_libraries(qam_tests PRIVATE GTest::GTest GTest::Main Modulator Demodulator NoiseAdder)

    foreach(TEST_FILE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_test(NAME ${TEST_NAME} COMMAND qam_tests --gtest_filter=${TEST_NAME}.*)
    endforeach()
endif()

add_custom_target(plot
    COMMAND python3 ${SCRIPTS_DIR}/plot_ber.py
    COMMENT "Plotting BER vs SNR"
    WORKING_DIRECTORY ${PROJECT_ROOT}
    VERBATIM
)

if(BUILD_APPLICATION)
    install(TARGETS qam_simulator DESTINATION bin)
    install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION include)
endif()
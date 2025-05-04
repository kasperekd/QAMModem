cmake_minimum_required(VERSION 3.20)
project(QAMSimulator LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опции проекта
option(ENABLE_SIMD "Enable SIMD optimizations (AVX2/FMA)" OFF)
option(BUILD_TESTS "Build unit tests with GoogleTest" OFF)
option(USE_PMR "Use std::pmr for memory management" ON)
option(BUILD_APPLICATION "Build main application" ON)

# Флаги для SIMD
if(ENABLE_SIMD)
    add_compile_options(-mavx2 -mfma)
endif()

# Корневые пути
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(INCLUDE_DIR ${PROJECT_ROOT}/include)
set(SRC_DIR ${PROJECT_ROOT}/src)
set(TESTS_DIR ${PROJECT_ROOT}/tests)
set(SCRIPTS_DIR ${PROJECT_ROOT}/scripts)

# Подключаем заголовочные файлы проекта
include_directories(${INCLUDE_DIR})

# Подключаем установленный GoogleTest
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
endif()

# --- Шаблонные библиотеки (в заголовочных файлах) ---
add_library(Modulator INTERFACE)
target_include_directories(Modulator INTERFACE ${INCLUDE_DIR})

# --- Статические библиотеки (реализация в .cpp) ---
if(BUILD_APPLICATION)
    # NoiseAdder
    add_library(NoiseAdder STATIC
        ${SRC_DIR}/noise/noise_adder.cpp
    )
    target_include_directories(NoiseAdder PUBLIC ${INCLUDE_DIR})
    if(ENABLE_SIMD)
        target_compile_definitions(NoiseAdder PRIVATE ENABLE_SIMD)
    endif()

    # Demodulator
    add_library(Demodulator STATIC
        ${SRC_DIR}/demodulator/demodulator_qam.cpp
    )
    target_include_directories(Demodulator PUBLIC ${INCLUDE_DIR})
    if(ENABLE_SIMD)
        target_compile_definitions(Demodulator PRIVATE ENABLE_SIMD)
    endif()

    # Pipeline
    add_library(Pipeline STATIC
        ${SRC_DIR}/pipeline/qam_simulator.cpp
    )
    target_include_directories(Pipeline PUBLIC ${INCLUDE_DIR})
    target_link_libraries(Pipeline PRIVATE Modulator NoiseAdder Demodulator)

    # Utils
    add_library(Utils STATIC
        ${SRC_DIR}/utils/random.cpp
        ${SRC_DIR}/utils/csv_writer.cpp
    )
    target_include_directories(Utils PUBLIC ${INCLUDE_DIR})

    # Исполняемый файл
    add_executable(qam_simulator ${SRC_DIR}/main.cpp)
    target_link_libraries(qam_simulator PRIVATE Pipeline Utils)
    if(USE_PMR)
        target_compile_definitions(qam_simulator PRIVATE USE_PMR)
    endif()
endif()

# --- Тесты ---
if(BUILD_TESTS)
    file(GLOB TEST_SOURCES "${TESTS_DIR}/*.cpp")
    add_executable(qam_tests ${TEST_SOURCES})
    target_link_libraries(qam_tests PRIVATE GTest::GTest GTest::Main Modulator)

    foreach(TEST_FILE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_test(NAME ${TEST_NAME} COMMAND qam_tests --gtest_filter=${TEST_NAME}.*)
    endforeach()
endif()

# --- График BER ---
add_custom_target(plot
    COMMAND python3 ${SCRIPTS_DIR}/plot_ber.py
    COMMENT "Plotting BER vs SNR"
    WORKING_DIRECTORY ${PROJECT_ROOT}
    VERBATIM
)

# --- Установка (только если собирается приложение)
if(BUILD_APPLICATION)
    install(TARGETS qam_simulator DESTINATION bin)
    install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION include)
endif()
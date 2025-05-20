cmake_minimum_required(VERSION 3.20)
project(QAMSimulator LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_APPLICATION "Build main application" ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(INCLUDE_DIR ${PROJECT_ROOT}/include)
set(SRC_DIR ${PROJECT_ROOT}/src)
set(SCRIPTS_DIR ${PROJECT_ROOT}/scripts)

include_directories(${INCLUDE_DIR})

add_library(QAMModulator INTERFACE)
target_include_directories(QAMModulator INTERFACE ${INCLUDE_DIR})

add_library(QAMDemodulator INTERFACE)
target_include_directories(QAMDemodulator INTERFACE ${INCLUDE_DIR})

add_library(QAMNoiseAdder INTERFACE)
target_include_directories(QAMNoiseAdder INTERFACE ${INCLUDE_DIR})

if(BUILD_APPLICATION)
    add_library(QAMUtils STATIC
        ${SRC_DIR}/utils/csv_writer.cpp
    )
    target_include_directories(QAMUtils PUBLIC ${INCLUDE_DIR})
endif()

if(BUILD_APPLICATION)
    add_library(QAMPipeline STATIC
        ${SRC_DIR}/pipeline/qam_simulator.cpp
    )
    target_include_directories(QAMPipeline PUBLIC ${INCLUDE_DIR})
    target_link_libraries(QAMPipeline PRIVATE
        QAMModulator 
        QAMDemodulator 
        QAMNoiseAdder
        QAMUtils
    )
endif()

if(BUILD_APPLICATION)
    add_executable(qam_simulator ${PROJECT_ROOT}/main.cpp)
    target_link_libraries(qam_simulator PRIVATE QAMPipeline) 

    install(TARGETS qam_simulator DESTINATION bin)
    install(DIRECTORY ${INCLUDE_DIR}/qam_simulator DESTINATION include)
endif()

add_custom_target(plot
    COMMAND python3 ${SCRIPTS_DIR}/plot_ber.py
    COMMENT "Plotting BER vs SNR"
    WORKING_DIRECTORY ${PROJECT_ROOT}
    VERBATIM
)
find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
    message(WARNING "Python3 interpreter not found, 'plot' target may not work.")
endif()
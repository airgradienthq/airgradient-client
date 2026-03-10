cmake_minimum_required(VERSION 3.10)
project(AirGradientPayloadEncoder VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags for embedded compatibility
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(ENCODER_SOURCES
    src/payload_encoder.cpp
)

set(ENCODER_HEADERS
    src/payload_types.h
    src/payload_encoder.h
)

# Library target
add_library(payload_encoder STATIC ${ENCODER_SOURCES})
target_include_directories(payload_encoder PUBLIC src)

# Example executable
add_executable(encoder_example examples/demo.cpp)
target_link_libraries(encoder_example PRIVATE payload_encoder)
target_include_directories(encoder_example PRIVATE src)

# Unity test framework - automatically download
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.0
)
FetchContent_GetProperties(unity)
if(NOT unity_POPULATED)
    FetchContent_Populate(unity)
    add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)
    target_include_directories(unity PUBLIC ${unity_SOURCE_DIR}/src)
endif()

# Enable testing
enable_testing()

# Add test subdirectory
add_subdirectory(test)

# Installation (optional)
install(TARGETS payload_encoder
    ARCHIVE DESTINATION lib
)

install(FILES ${ENCODER_HEADERS}
    DESTINATION include/airgradient
)

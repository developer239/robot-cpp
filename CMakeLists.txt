cmake_minimum_required(VERSION 3.21)

project(RobotCPP)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library name
set(LIB_NAME RobotCPP)

# Option to build tests (OFF by default)
option(BUILD_ROBOT_TESTS "Build the RobotCPP tests" OFF)

# Only find dependencies and add GoogleTest if tests are enabled
if(BUILD_ROBOT_TESTS)
    # Add GoogleTest
    add_subdirectory(externals/googletest)
    enable_testing()

    # Find SDL2 for tests
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/sdl2/")
    find_package(SDL2 REQUIRED)
endif()

# Compiler-specific options
if(MSVC)
    # MSVC flags
    add_compile_options(/W4 /MP)
else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Common source files
set(COMMON_SOURCES
        src/ActionRecorder.h
        src/types.h
        src/Mouse.h
        src/Mouse.cpp
        src/Utils.h
        src/Utils.cpp
        src/Keyboard.cpp
        src/Keyboard.h
        src/Screen.cpp
        src/Screen.h)

# External dependencies
set(SOURCES_LODEPNG
        externals/lodepng/lodepng.cpp
        externals/lodepng/lodepng.h)

# Platform-specific components
if(APPLE)
    list(APPEND PLATFORM_SOURCES src/EventHookMacOS.h)
    find_library(CARBON_LIBRARY Carbon)
    mark_as_advanced(CARBON_LIBRARY)
    list(APPEND PLATFORM_LIBRARIES ${CARBON_LIBRARY})
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES src/EventHookWindows.h)
endif()

# Define the main library
add_library(${LIB_NAME} STATIC ${COMMON_SOURCES} ${PLATFORM_SOURCES} ${SOURCES_LODEPNG})
target_include_directories(${LIB_NAME} PUBLIC src PRIVATE externals/lodepng)
target_link_libraries(${LIB_NAME} ${PLATFORM_LIBRARIES})

# Set output directory for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# Add the tests directory only if tests are enabled
if(BUILD_ROBOT_TESTS)
    add_subdirectory(tests)
endif()

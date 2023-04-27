cmake_minimum_required(VERSION 3.24)

project(RobotCPP)

set(CMAKE_CXX_STANDARD 23)

set(LIB_NAME RobotCPP)
set(LIB_NAME_TEST ${LIB_NAME}Test)

# Find GoogleTest
add_subdirectory(externals/googletest)
enable_testing()

# Configure test executable
set(SOURCES_TEST src/types.h src/Mouse.h src/Mouse.cpp src/Mouse.test.cpp src/Utils.h src/Utils.cpp src/Keyboard.cpp src/Keyboard.h src/Screen.cpp src/Screen.h)
set(SOURCES_LODEPNG externals/lodepng/lodepng.cpp externals/lodepng/lodepng.h)
add_executable(${LIB_NAME_TEST} ${SOURCES_TEST} ${SOURCES_LODEPNG})
target_include_directories(${LIB_NAME_TEST} PRIVATE externals/lodepng)
target_link_libraries(${LIB_NAME_TEST} PRIVATE gtest gmock gmock_main "-framework Carbon")

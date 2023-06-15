cmake_minimum_required(VERSION 3.21)

project(RobotCPP)

set(CMAKE_CXX_STANDARD 23)
set(LIB_NAME RobotCPP)

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

set(SOURCES_LODEPNG
        externals/lodepng/lodepng.cpp
        externals/lodepng/lodepng.h)

if (APPLE)
    list(APPEND PLATFORM_SOURCES src/EventHookMacOS.h)
    find_library(CARBON_LIBRARY Carbon)
    mark_as_advanced(CARBON_LIBRARY)
    list(APPEND PLATFORM_LIBRARIES ${CARBON_LIBRARY})
elseif (WIN32)
    list(APPEND PLATFORM_SOURCES src/EventHookWindows.h)
endif ()

add_library(${LIB_NAME} STATIC ${COMMON_SOURCES} ${PLATFORM_SOURCES} ${SOURCES_LODEPNG})
target_include_directories(${LIB_NAME} PUBLIC src PRIVATE externals/lodepng)
target_link_libraries(${LIB_NAME} ${PLATFORM_LIBRARIES})

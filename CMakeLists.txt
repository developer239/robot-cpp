cmake_minimum_required(VERSION 3.24)

project(RobotCPP)

set(CMAKE_CXX_STANDARD 23)

set(LIB_NAME RobotCPP)
set(LIB_NAME_TEST ${LIB_NAME}Test)

add_subdirectory(externals/googletest)
enable_testing()

set(SOURCES src/types.h src/Mouse.h src/Mouse.cpp src/Utils.h src/Utils.cpp src/Keyboard.cpp src/Keyboard.h src/Screen.cpp src/Screen.h)
set(SOURCES_LODEPNG externals/lodepng/lodepng.cpp externals/lodepng/lodepng.h)
add_library(${LIB_NAME} STATIC ${SOURCES} ${SOURCES_LODEPNG})
target_include_directories(${LIB_NAME} PUBLIC src PRIVATE externals/lodepng)

if (APPLE)
    target_link_libraries(${LIB_NAME}
            "-framework Carbon"
            )
endif ()

set(SOURCES_TEST src/Mouse.test.cpp)
add_executable(${LIB_NAME_TEST} ${SOURCES_TEST})
target_link_libraries(${LIB_NAME_TEST} PRIVATE ${LIB_NAME} gtest gmock gmock_main)

if (APPLE)
    target_link_libraries(${LIB_NAME_TEST}
            PRIVATE
            "-framework Carbon"
            )
endif ()

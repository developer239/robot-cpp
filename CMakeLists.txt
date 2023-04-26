cmake_minimum_required(VERSION 3.24)
project(robot_cpp)

set(CMAKE_CXX_STANDARD 23)

add_executable(robot_cpp src/types.h src/Mouse.h src/Mouse.cpp src/Mouse.test.cpp)

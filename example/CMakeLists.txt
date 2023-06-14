cmake_minimum_required(VERSION 3.21)

project(RobotCPPExample)

set(CMAKE_CXX_STANDARD 23)
set(APP_NAME RobotCPPExample)

add_subdirectory(../ ${CMAKE_CURRENT_BINARY_DIR}/RobotCPP)
add_executable(MouseExample main.cpp)

target_link_libraries(MouseExample PRIVATE RobotCPP)

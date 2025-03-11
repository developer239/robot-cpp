set(TEST_NAME RobotCPPTest)
set(SDL_TEST_NAME RobotCPPSDLTest)

# We keep the gtest reference in the CMake setup as requested
# But we don't need to create the actual test executable
# Instead, just ensure gtest is available for other targets if needed
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # GTest is already included via add_subdirectory in the main CMakeLists.txt
    # We don't need to do anything here
endif()

# SDL2 Functional Tests - Only keeping mouse drag test
set(SDL_TEST_SOURCES
        sdl/SDLTestApp.cpp
        sdl/TestElements.h
        sdl/MouseTests.h
)

add_executable(${SDL_TEST_NAME} ${SDL_TEST_SOURCES})
target_link_libraries(${SDL_TEST_NAME} PRIVATE
        RobotCPP
        SDL2::SDL2
)

# Set output directory to be consistent across build types
set_target_properties(${SDL_TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# Copy test assets
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Add a custom command to build the SDL test executable as part of ALL target
add_custom_target(build_sdl_tests ALL DEPENDS ${SDL_TEST_NAME})

# Add the SDL test as a test
add_test(NAME SDLFunctionalTests
        COMMAND ${SDL_TEST_NAME} --run-tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add another test configuration for interactive mode
add_test(NAME SDLInteractiveTests
        COMMAND ${SDL_TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_tests_properties(SDLInteractiveTests PROPERTIES DISABLED TRUE)

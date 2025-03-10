set(TEST_NAME RobotCPPTest)
set(SDL_TEST_NAME RobotCPPSDLTest)

# Unit Tests
set(TEST_SOURCES
        unit/MouseTest.cpp
        unit/KeyboardTest.cpp
        unit/ScreenTest.cpp
)

add_executable(${TEST_NAME} ${TEST_SOURCES})
target_link_libraries(${TEST_NAME} PRIVATE
        gtest
        gmock
        gtest_main
        RobotCPP
)

add_test(NAME UnitTests COMMAND ${TEST_NAME})

# SDL2 Functional Tests
set(SDL_TEST_SOURCES
        sdl/SDLTestApp.cpp
        sdl/TestElements.h
        sdl/MouseTests.h
        sdl/KeyboardTests.h
        sdl/ScreenTests.h
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
        COMMAND ${SDL_TEST_NAME} --headless --run-tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add another test configuration for interactive mode
add_test(NAME SDLInteractiveTests
        COMMAND ${SDL_TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_tests_properties(SDLInteractiveTests PROPERTIES DISABLED TRUE)

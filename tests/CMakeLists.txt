set(SDL_TEST_NAME RobotCPPSDLTest)

# Find test dependencies
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # GTest is already included via add_subdirectory in the main CMakeLists.txt
endif()

# Define test sources
set(SDL_TEST_SOURCES
        sdl/SDLTestMain.cpp
        sdl/MouseTests.cpp
        sdl/TestElements.h
        sdl/TestContext.h
        sdl/TestConfig.h
        sdl/RobotSDLTestFixture.h
)

# Create test executable
add_executable(${SDL_TEST_NAME} ${SDL_TEST_SOURCES})

# Link dependencies
target_link_libraries(${SDL_TEST_NAME} PRIVATE
        RobotCPP
        SDL2::SDL2
        gtest
        gtest_main
)

# Set output directory for test targets
set_target_properties(${SDL_TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# Add a custom command to build the SDL test executable
add_custom_target(build_sdl_tests ALL DEPENDS ${SDL_TEST_NAME})

# Add automated tests
add_test(
        NAME SDLFunctionalTests
        COMMAND ${SDL_TEST_NAME} --run-tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

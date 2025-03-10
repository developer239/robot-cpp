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

# Copy test assets
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/tests)

add_test(NAME FunctionalTests
        COMMAND ${SDL_TEST_NAME} --headless --run-tests)

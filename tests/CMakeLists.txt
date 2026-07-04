# GoogleTest via FetchContent so the framework is not a required system package
# and the main dependency list stays clean.
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Portable unit tests: no OS interaction, always safe in CI ─────────────────
# These link the public headers and a mock backend; they never call a real OS
# injection path, so they run headless on any CI runner.
add_executable(robot_unit_tests
    unit/GeometryTests.cpp
    unit/ModifiersTests.cpp
    unit/KeyTests.cpp
    unit/Utf8Tests.cpp
    unit/RecorderTests.cpp
    unit/ScreenLogicTests.cpp
    unit/MouseSequenceTests.cpp
    unit/support/MockBackend.h
)
target_link_libraries(robot_unit_tests PRIVATE robot::robot gtest_main)
target_include_directories(robot_unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/support)

include(GoogleTest)
gtest_discover_tests(robot_unit_tests)

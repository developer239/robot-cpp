cmake_minimum_required(VERSION 3.24)

project(robot-cpp
    VERSION 0.2.0
    DESCRIPTION "Cross-platform keyboard, mouse, and screen automation"
    LANGUAGES CXX
)

# Honour CMAKE_CXX_STANDARD on targets and keep this a subproject-friendly build.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# Detect whether we are the top-level project so options don't leak into parents.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ROBOT_IS_TOPLEVEL ON)
else()
    set(ROBOT_IS_TOPLEVEL OFF)
endif()

option(ROBOT_BUILD_TESTS "Build robot-cpp tests" ${ROBOT_IS_TOPLEVEL})
option(ROBOT_BUILD_EXAMPLES "Build robot-cpp examples" ${ROBOT_IS_TOPLEVEL})
option(ROBOT_WERROR "Treat warnings as errors" OFF)
option(ROBOT_LINUX_ENABLE_UINPUT "Build the Linux uinput backend" ON)

# ── Warnings interface ────────────────────────────────────────────────────────
# A dedicated interface target carries warning flags so they attach per-target
# instead of polluting the whole directory via add_compile_options.
add_library(robot_warnings INTERFACE)
if(MSVC)
    target_compile_options(robot_warnings INTERFACE /W4 /permissive- /MP)
    if(ROBOT_WERROR)
        target_compile_options(robot_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(robot_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(ROBOT_WERROR)
        target_compile_options(robot_warnings INTERFACE -Werror)
    endif()
endif()

# ── Vendored PNG encoder (private) ────────────────────────────────────────────
add_library(robot_lodepng STATIC externals/lodepng/lodepng.cpp)
target_include_directories(robot_lodepng
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/externals/lodepng>
        $<INSTALL_INTERFACE:include/robot/external/lodepng>
)
set_target_properties(robot_lodepng PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── Portable sources (compiled on every platform) ─────────────────────────────
set(ROBOT_COMMON_SOURCES
    src/common/Session.cpp
    src/common/Keyboard.cpp
    src/common/Mouse.cpp
    src/common/Screen.cpp
    src/common/Image.cpp
    src/common/Recorder.cpp
    src/common/EventTap.cpp
)

# ── Platform selection: exactly one directory is compiled ─────────────────────
# This is the single compile boundary. No consumer translation unit contains a
# platform #ifdef; the platform is chosen here, once.
if(APPLE)
    set(ROBOT_PLATFORM_SOURCES
        src/platform/macos/MacBackendFactory.cpp
        src/platform/macos/MacKeyMap.cpp
        src/platform/macos/MacKeyboardBackend.cpp
        src/platform/macos/MacMouseBackend.cpp
        src/platform/macos/MacScreenBackend.cpp
        src/platform/macos/MacEventTapBackend.cpp
    )
elseif(WIN32)
    set(ROBOT_PLATFORM_SOURCES
        src/platform/windows/WinBackendFactory.cpp
        src/platform/windows/WinDpi.cpp
        src/platform/windows/WinKeyMap.cpp
        src/platform/windows/WinKeyboardBackend.cpp
        src/platform/windows/WinMouseBackend.cpp
        src/platform/windows/WinScreenBackend.cpp
        src/platform/windows/WinEventTapBackend.cpp
    )
elseif(UNIX AND NOT APPLE)
    set(ROBOT_PLATFORM_SOURCES
        src/platform/linux/LinuxBackendFactory.cpp
        src/platform/linux/LinuxPlatformBackend.cpp
        src/platform/linux/X11Display.cpp
        src/platform/linux/X11KeyMap.cpp
        src/platform/linux/X11KeyboardBackend.cpp
        src/platform/linux/X11MouseBackend.cpp
        src/platform/linux/X11ScreenBackend.cpp
        src/platform/linux/X11EventTapBackend.cpp
    )
    if(ROBOT_LINUX_ENABLE_UINPUT)
        list(APPEND ROBOT_PLATFORM_SOURCES
            src/platform/linux/UinputBackend.cpp
            src/platform/linux/LinuxEvdevKeymap.cpp
        )
    endif()
else()
    message(FATAL_ERROR "robot-cpp: unsupported platform")
endif()

# ── Main library ──────────────────────────────────────────────────────────────
add_library(robot ${ROBOT_COMMON_SOURCES} ${ROBOT_PLATFORM_SOURCES})
add_library(robot::robot ALIAS robot)

target_compile_features(robot PUBLIC cxx_std_23)

# Public headers under include/; implementation-only headers under src/ are
# private. The generator expression keeps install and build-tree includes correct.
target_include_directories(robot
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# lodepng and the warnings flags are implementation details: PRIVATE, so they do
# not propagate to consumers of the library.
target_link_libraries(robot
    PRIVATE
        robot_lodepng
        robot_warnings
)

set_target_properties(robot PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── Platform dependencies, scoped per target ──────────────────────────────────
if(APPLE)
    find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
    # ApplicationServices types appear only in .cpp files (behind the backend
    # interfaces), so the dependency is PRIVATE. Carbon is intentionally NOT used.
    target_link_libraries(robot PRIVATE ${APPLICATION_SERVICES})
elseif(WIN32)
    # user32/gdi32 for SendInput, hooks, GDI capture. Shcore and the DPI context
    # API are resolved at runtime (see WinDpi.cpp / WinScreenBackend.cpp), so they
    # are not linked here.
    target_link_libraries(robot PRIVATE user32 gdi32)
    target_compile_definitions(robot PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT TARGET X11::Xtst)
        message(FATAL_ERROR "robot-cpp: XTest development files are required")
    endif()
    if(NOT TARGET X11::Xrandr)
        message(FATAL_ERROR "robot-cpp: XRandR development files are required")
    endif()
    # X11 headers are used only in platform .cpp files -> PRIVATE. XTest and
    # Xrandr components ship with X11; XRecord lives in libXtst as well.
    target_link_libraries(robot PRIVATE
        X11::X11 X11::Xtst X11::Xrandr)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(ROBOT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Examples ──────────────────────────────────────────────────────────────────
if(ROBOT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── Install / export ──────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS robot robot_lodepng robot_warnings
    EXPORT robotTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/robot DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES
    externals/lodepng/lodepng.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/robot/external/lodepng
)

install(EXPORT robotTargets
    FILE robotTargets.cmake
    NAMESPACE robot::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/robot
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/robotConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
# A minimal package config that re-finds the platform deps on the consumer side.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/robotConfig.cmake"
"include(CMakeFindDependencyMacro)\n\
if(UNIX AND NOT APPLE)\n\
  find_dependency(X11)\n\
  if(NOT TARGET X11::Xtst)\n\
    message(FATAL_ERROR \"robot-cpp: XTest development files are required\")\n\
  endif()\n\
  if(NOT TARGET X11::Xrandr)\n\
    message(FATAL_ERROR \"robot-cpp: XRandR development files are required\")\n\
  endif()\n\
endif()\n\
include(\"\${CMAKE_CURRENT_LIST_DIR}/robotTargets.cmake\")\n")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/robotConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/robotConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/robot
)

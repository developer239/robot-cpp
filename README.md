# robot-cpp

[![CI](https://github.com/developer239/robot-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/developer239/robot-cpp/actions/workflows/ci.yml)

robot-cpp is a C++23 library for programmatic keyboard, mouse, and screen control on macOS, Windows, and Linux. It injects synthetic input, captures the screen at native resolution, and records and replays global input events, all behind a single `Session` object that owns the platform backend and reports what it can and cannot do in the current environment.

## Why it exists

Input-automation libraries in the [robotjs](https://github.com/octalmage/robotjs) lineage share a set of correctness gaps. They map characters through a hard-coded US-keyboard table, so capitals and symbols type wrong on other layouts, or cannot be typed at all. They treat the primary display as the whole screen, so multi-monitor and HiDPI setups break. They conflate logical desktop coordinates with device pixels, so capture buffers and cursor positions disagree on Retina and DPI-scaled displays. And they fail silently when the operating system withholds a capability or a permission.

robot-cpp is built around the distinctions those libraries collapse:

- **Physical keys are separate from text.** A key is identified by its position; typing text injects Unicode directly and is layout-independent by construction.
- **Logical coordinates are separate from device pixels.** Each display carries its own DPI scale factor and both coordinate spaces.
- **Capabilities are reported and limits are hard errors.** Every fallible call returns a typed result; nothing degrades into a silent no-op.
- **There is no global state.** Everything hangs off an explicit `Session`, so lifetimes are clear and the whole stack is testable.

## What it does

- Move the cursor (absolute or smoothly interpolated), click, double-click, drag, and scroll, with buttons through X1/X2 and line- or pixel-unit scrolling.
- Press and release physical keys by position, build modifier chords, and type arbitrary Unicode text independent of the active keyboard layout.
- Enumerate every display with its own scale factor and both logical and physical bounds, capture any region or a whole monitor at native pixel resolution, sample individual pixels, and encode captures as PNG.
- Record global mouse and keyboard input and replay it with its original timing.
- Report per-environment capabilities and return typed errors instead of failing silently.

## Supported platforms

| Platform          | Backend                       | Injection                       | Capture | Recording |
| ----------------- | ----------------------------- | ------------------------------- | ------- | --------- |
| macOS             | Quartz (CoreGraphics)         | yes (needs Accessibility)       | yes (needs Screen Recording) | yes (needs Accessibility) |
| Windows           | SendInput + GDI               | yes                             | yes     | yes       |
| Linux (X11)       | XTest + XRandR + XRecord      | yes                             | yes     | yes       |
| Linux (Wayland)   | uinput (opt-in)               | keyboard + relative mouse only  | no      | no        |

Wayland does not expose a protocol for an unprivileged client to inject input, warp the cursor, or capture the screen. Under a native Wayland session robot-cpp either runs through Xwayland (the X11 backend) or through the kernel-level uinput backend, with the limits above reported explicitly. See [Platform limitations](#platform-limitations) for the details.

## Requirements

- A C++23 toolchain: Apple Clang 17+ (Xcode 16+), Clang 18+, or GCC 14+.
- CMake 3.24 or newer.
- **macOS:** no extra packages; the library links `ApplicationServices` (Carbon is not used).
- **Windows:** the Windows SDK (`user32`, `gdi32`).
- **Linux:** X11 development packages - on Debian/Ubuntu, `libx11-dev`, `libxtst-dev`, `libxrandr-dev`. The optional uinput backend additionally needs kernel `uinput` support and write access to `/dev/uinput`.

lodepng is vendored as a git submodule, so fetch submodules recursively (below). GoogleTest is downloaded automatically when tests are enabled, and SDL2 is only needed for the opt-in interactive tests.

## Installation

Add the library as a submodule and pull its dependencies:

```bash
git submodule add https://github.com/developer239/robot-cpp externals/robot-cpp
git submodule update --init --recursive
```

Link it from your CMake project:

```cmake
add_subdirectory(externals/robot-cpp)
target_link_libraries(your_target PRIVATE robot::robot)
```

Then include the umbrella header, or pull in individual headers to keep dependencies tight:

```cpp
#include "robot/Robot.h"
```

Alternatively, install the library and consume it as a package:

```cmake
find_package(robot REQUIRED)
target_link_libraries(your_target PRIVATE robot::robot)
```

## Core concepts

### A session owns the backend

All state lives on a `Session`; there is no global input state. `Session::create()` performs the fallible setup - selecting a backend, checking permissions, probing the display server - and returns `std::expected<std::unique_ptr<Session>, robot::Error>`. On success you get a fully formed session; on failure a specific error, never a half-initialized object.

The session hands out references to four subsystems, `keyboard()`, `mouse()`, `screen()`, and `eventTap()`, plus a `capabilities()` report. Those references borrow the session-owned backend, so the session must outlive them; it is non-copyable and non-movable to keep them stable.

### Physical keys versus text

This is the most important distinction in the API, and confusing the two is the root of the "only works on US keyboards" problem.

- A `robot::Key` names a **physical key by position**. Its value is the USB HID usage id, independent of layout. `Key::A` is the key in the US-QWERTY A position; on an AZERTY layout that same physical key produces `q`. Use physical keys for shortcuts, chords, and games - anything positional, like WASD movement or Ctrl+key.
- **Text** (`typeText`, `typeChar`) injects Unicode directly and is layout-independent by construction. This is the only correct way to produce specific characters - capitals, symbols, accented letters, CJK, emoji - because it does not assume any key-to-character mapping. Do not try to spell characters out of key presses; that only works on the one layout you hard-coded for.

Modifier semantics differ per platform and the library does not remap them for you: `Modifier::Meta` is Command on macOS and the Super/Windows key on Windows and Linux, and `Modifier::Alt` is Option on macOS. So a cross-platform "select all" is `Meta+A` on macOS and `Control+A` on Windows and Linux.

### Logical versus physical coordinates

- **Logical coordinates** (`LogicalPoint`, `LogicalSize`, `LogicalRect`) are DPI-independent desktop units - macOS points, Windows DIPs. Cursor movement and position operate here, so behaviour is the same across displays of different density.
- **Physical coordinates** (`PhysicalPoint`, `PhysicalSize`, `PhysicalRect`) are device pixels. Screen capture and pixel access operate here.

The two are distinct types, and conversion is always explicit and goes through a scale factor. Each `Monitor` carries its own `scaleFactor` (2.0 on a typical Retina panel, 1.5 at 150% on Windows), both coordinate spaces, and can sit at a negative origin when placed left of or above the primary. A 100x100 logical capture on a 2x display yields a 200x200 image, and that pixel count is the truth about what was captured.

### Capabilities and explicit errors

Query `capabilities()` once after creating a session and branch on the flags. A `false` flag means the corresponding call returns `robot::ErrorCode::Unsupported` or `PermissionDenied`, never a silent no-op. Every fallible operation returns `std::expected<T, robot::Error>`, where `Error` carries an `ErrorCode` for programmatic handling and a human-readable `message`. Pixel-precise scrolling on a backend that lacks it, warping the cursor under unprivileged Wayland, injecting an X1 button where the OS cannot express it - all return a specific error you can see and handle.

## Quick start

```cpp
#include <print>
#include "robot/Robot.h"

int main() {
  auto session = robot::Session::create();
  if (!session) {
    std::println("robot-cpp unavailable: {}", session.error().message);
    return 1;
  }

  robot::Keyboard& keyboard = (*session)->keyboard();
  robot::Mouse& mouse = (*session)->mouse();

  if (auto r = mouse.moveSmooth({400.0, 300.0}); !r) {
    std::println("move failed: {}", r.error().message);
  }
  if (auto r = keyboard.typeText("Hello, 世界! 🙂"); !r) {
    std::println("type failed: {}", r.error().message);
  }
  return 0;
}
```

Every action returns `std::expected<T, robot::Error>`. The snippets below omit the `if (auto r = ...; !r)` checks shown above for brevity; handle the results the same way in real code.

## Keyboard

```cpp
// Layout-independent Unicode text (any script, symbols, emoji):
keyboard.typeText("café ☕ 日本語 🙂");

// Human-paced typing (randomized inter-key delay; a fixed seed makes it reproducible):
keyboard.typeTextHumanLike("dear reviewer,");

// A single physical key, by position:
keyboard.tap(robot::Key::Enter);

// Modifier chords: the modifier is held around the key and released in reverse order.
keyboard.tap(robot::Key::C, robot::Modifiers{robot::Modifier::Control});
keyboard.tap(robot::Key::S, robot::Modifier::Control | robot::Modifier::Shift);

// Hold a key down and release it later (for example, movement in a game):
keyboard.press(robot::Key::W);
// ... later ...
keyboard.release(robot::Key::W);
```

## Mouse

Mouse control operates in global logical coordinates (the virtual desktop shared by all monitors).

```cpp
mouse.move({800.0, 450.0});          // absolute warp in logical coordinates
mouse.moveSmooth({100.0, 100.0});    // interpolated over a duration

mouse.click();                       // left click at the current position
mouse.click(robot::MouseButton::Right);
mouse.doubleClick();                 // reported to the OS as a real double-click
mouse.click(robot::MouseButton::X1); // needs capabilities().supportsExtraMouseButtons

mouse.drag({500.0, 500.0});          // press, move (delivered as a drag), release
mouse.dragSmooth({500.0, 500.0});

mouse.scroll(robot::ScrollDelta::lines(3));     // three notches up
mouse.scroll(robot::ScrollDelta::pixels(-120)); // needs supportsHighResolutionScroll

auto pos = mouse.position();         // std::expected<robot::LogicalPoint, Error>
```

Scroll sign convention, applied before any operating-system "natural scrolling" setting: `vertical > 0` scrolls up, `horizontal > 0` scrolls right. Natural scrolling may invert the visible direction; that is a user preference the library does not hide.

## Screen

Capture regions are specified in device pixels, because that is the only unambiguous unit across mixed-density displays. Use a monitor's `physicalBounds`, or `captureMonitor`, to avoid doing the scale math by hand.

```cpp
robot::Screen& screen = (*session)->screen();

// Every display, primary first, each with its own scale factor and both coordinate spaces:
if (auto monitors = screen.monitors()) {
  for (const robot::Monitor& m : *monitors) {
    std::println("{}: {}x{} physical, scale {:.2f}{}", m.name,
                 m.physicalBounds.size.width, m.physicalBounds.size.height,
                 m.scaleFactor, m.isPrimary ? " (primary)" : "");
  }
}

// Capture the primary display at native pixel resolution and save a PNG:
if (auto primary = screen.primaryMonitor()) {
  if (auto image = screen.captureMonitor(primary->id)) {
    image->savePng("primary.png");
  }
}

// Capture an explicit device-pixel region (origin, size):
auto region = screen.capture(robot::PhysicalRect{{0, 0}, {1920, 1080}});

// Sample one pixel at a device-pixel coordinate:
if (auto color = screen.pixel({100, 200})) {
  std::println("rgba({}, {}, {}, {})", color->r, color->g, color->b, color->a);
}
```

## Recording and replay

A global event tap observes all mouse and keyboard activity and forwards it as normalized events, which a `Recorder` stamps with elapsed time. Recording is a privileged, platform-limited capability, so check `canRecordEvents` first. Key events are captured as physical keys, so a recording replays by position and is layout-independent.

```cpp
#include <chrono>
#include <thread>

if (!(*session)->capabilities().canRecordEvents) {
  std::println("Global recording is not available on this backend.");
  return 1;
}

robot::Recorder recorder;
robot::EventTap& tap = (*session)->eventTap();

// start() blocks on the OS event loop until stop(), so run it on its own thread.
std::thread recording([&] {
  if (auto r = tap.start([&](const robot::InputEvent& e) { recorder.capture(e); });
      !r) {
    std::println("tap error: {}", r.error().message);
  }
});

std::this_thread::sleep_for(std::chrono::seconds(5));
tap.stop();
recording.join();

std::println("captured {} events", recorder.events().size());

// Replay honours the recorded gaps; timeScale and maxGap are available via ReplayOptions.
if (auto r = recorder.replay(**session); !r) {
  std::println("replay failed: {}", r.error().message);
}
```

## Building and running tests

```bash
cmake -S . -B build -DROBOT_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The unit tests exercise the entire portable stack - chord building, UTF-8 decoding, smooth-move sequencing, recorder timing, and the screen facade math - against a mock backend, so they do no real OS injection and run headless in normal CI.

The interactive tests drive the real cursor and keyboard against an on-screen SDL window and cannot run on a headless runner, so they are opt-in and never part of the default test run:

```bash
cmake -S . -B build -DROBOT_BUILD_INTERACTIVE_TESTS=ON
cmake --build build -j
ctest --test-dir build -R InteractiveInjection --output-on-failure
```

CMake options:

| Option                        | Default            | Effect                                              |
| ----------------------------- | ------------------ | --------------------------------------------------- |
| `ROBOT_BUILD_TESTS`           | on when top-level  | Build the portable unit tests.                      |
| `ROBOT_BUILD_INTERACTIVE_TESTS` | off              | Build the SDL injection tests (needs a live display). |
| `ROBOT_BUILD_EXAMPLES`        | on when top-level  | Build the example programs.                         |
| `ROBOT_WERROR`                | off                | Treat warnings as errors.                           |
| `ROBOT_LINUX_ENABLE_UINPUT`   | on                 | Build the Linux uinput backend.                     |

## Platform limitations

These are inherent to each platform and are reported through `capabilities()` and typed errors rather than hidden behind silent fallbacks.

### macOS

Injection and global recording require the Accessibility permission; screen capture requires the Screen Recording permission. Grant them in System Settings under Privacy & Security. Until they are granted, `capabilities()` reports the corresponding abilities as unavailable and the calls return `PermissionDenied`. To fail up front instead of at first use, construct the session with the permission preflight:

```cpp
robot::SessionOptions options;
options.requireInputPermission = true;    // Accessibility
options.requireCapturePermission = true;  // Screen Recording
auto session = robot::Session::create(options);
```

Screen capture uses CoreGraphics APIs that are deprecated (still functional) on macOS 14+. Migrating to ScreenCaptureKit is planned and is isolated to the macOS screen backend.

### Windows

No runtime permission is required for injection or GDI capture from an interactive desktop session. User Interface Privilege Isolation can still block injection into a window running at higher integrity than the calling process; that surfaces as a failed injection rather than a capability flag. High-resolution (pixel-unit) scrolling is granular but not sub-notch, because true per-pixel wheel data is a Precision Touchpad driver feature and is not available to synthetic input.

### Linux

Under X11 the library has full injection, capture, and recording through XTest, XRandR, and XRecord. Two X11 limits are reported explicitly: core-protocol scrolling is discrete wheel steps, so pixel-unit scrolling returns an error, and X exposes no per-monitor logical scaling at the core level, so `scaleFactor` is reported as 1.0.

Under a native Wayland session, an unprivileged client cannot inject input, warp or read the cursor, or capture the screen, because Wayland provides no protocol for it. `Session::create()` detects this and returns a specific error naming the two options:

- Run under Xwayland (set `DISPLAY`); the X11 backend then drives X11 and Xwayland clients.
- Construct the session with the kernel-level backend:

```cpp
robot::SessionOptions options;
options.linuxBackend = robot::LinuxBackend::Uinput;
auto session = robot::Session::create(options);
```

uinput works under Wayland but injects relative pointer motion only: there is no absolute cursor warp, no pointer-position read, no screen capture, no monitor enumeration, and no Unicode text (the kernel interface carries no layout). It requires write access to `/dev/uinput` (root, or a udev rule granting the input group). Every one of these limits is reflected in `capabilities()`.

## Examples

Runnable programs live in `examples/` and are built when `ROBOT_BUILD_EXAMPLES` is on. Each is a small, complete reference you can copy from:

- `type_text.cpp` - the create-then-check-capabilities pattern, plus the physical-key versus Unicode-text split.
- `capture_screen.cpp` - density-correct capture of the primary display, saved as PNG.
- `record_replay.cpp` - recording global input for a few seconds and replaying it with its original timing.

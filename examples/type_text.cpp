#include <print>

#include "robot/Robot.h"

// Demonstrates the create-then-check-capabilities pattern and the physical-key
// vs Unicode-text split. Every fallible call is handled explicitly; nothing
// silently no-ops.
int main() {
  auto session = robot::Session::create();
  if (!session) {
    std::println("Cannot start: {}", session.error().message);
    return 1;
  }

  const auto& caps = (*session)->capabilities();
  std::println("Backend: {}", caps.backendName);
  if (!caps.canInjectKeyboard) {
    std::println("Keyboard injection unavailable in this environment.");
    return 1;
  }

  robot::Keyboard& keyboard = (*session)->keyboard();

  // Layout-independent Unicode text, including characters absent from a US
  // keyboard.
  if (auto r = keyboard.typeText(
          "Hello, \xE4\xB8\x96\xE7\x95\x8C! \xF0\x9F\x99\x82\n"
      );
      !r) {
    std::println("typeText failed: {}", r.error().message);
    return 1;
  }

  // A positional chord: Command/Ctrl + A (select all). This is a physical-key
  // operation, deliberately not spelled as characters.
  if (auto r = keyboard.tap(robot::Key::A, robot::Modifiers{robot::Modifier::Meta});
      !r) {
    std::println("chord failed: {}", r.error().message);
    return 1;
  }

  return 0;
}

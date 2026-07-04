#pragma once

#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace robot {

// Physical mouse buttons. X1/X2 are the "back"/"forward" side buttons on most
// modern mice; a backend reports whether it can inject them via
// Capabilities::supportsExtraMouseButtons and fails with ErrorCode::Unsupported
// rather than silently dropping them.
enum class MouseButton : std::uint8_t {
  Left = 0,
  Right = 1,
  Middle = 2,
  X1 = 3,  // "Back" side button.
  X2 = 4,  // "Forward" side button.
};

constexpr std::string_view toString(const MouseButton button) {
  switch (button) {
    case MouseButton::Left: return "Left";
    case MouseButton::Right: return "Right";
    case MouseButton::Middle: return "Middle";
    case MouseButton::X1: return "X1";
    case MouseButton::X2: return "X2";
  }
  std::abort();
}

// Whether a button event presses or releases. A click is Down then Up. A double
// click issues two clicks with clickCount == 2 reported to the OS so native
// double-click detection fires (see IMouseBackend::button).
enum class ButtonAction : std::uint8_t { Down, Up };

constexpr std::string_view toString(const ButtonAction action) {
  switch (action) {
    case ButtonAction::Down: return "Down";
    case ButtonAction::Up: return "Up";
  }
  std::abort();
}

}  // namespace robot

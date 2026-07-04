#pragma once

#include <chrono>
#include <variant>

#include "robot/Geometry.h"
#include "robot/Key.h"
#include "robot/MouseButton.h"
#include "robot/Scroll.h"

namespace robot {

// Normalized input events. A platform event tap translates native events into
// these library types before they reach any portable code, so the recorder and
// replay logic are OS-independent and unit-testable.
//
// Key events carry a physical Key (position), not a character. Recording and
// replaying physical keys is layout-independent by construction: replay presses
// the same physical key regardless of the layout active at replay time. Text
// that depended on a specific character belongs to Keyboard::typeText, not here.

struct KeyEvent {
  Key key = Key::Unknown;
  bool down = false;  // true = press, false = release.
};

struct MouseMoveEvent {
  LogicalPoint position;
};

struct MouseButtonEvent {
  MouseButton button = MouseButton::Left;
  bool down = false;
  LogicalPoint position;
};

struct ScrollEvent {
  ScrollDelta delta;
  LogicalPoint position;
};

using InputEvent =
    std::variant<KeyEvent, MouseMoveEvent, MouseButtonEvent, ScrollEvent>;

// An input event stamped with the time elapsed since recording began. Absolute
// timestamps are avoided so a timeline replays identically regardless of when it
// is replayed.
struct RecordedEvent {
  std::chrono::milliseconds timestamp{0};
  InputEvent event;
};

}  // namespace robot
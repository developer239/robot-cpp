#pragma once

#include <expected>

#include "robot/Error.h"
#include "robot/Key.h"

namespace robot::backend {

// Native keyboard injection. Deliberately minimal: three atomic operations from
// which the portable Keyboard facade composes taps, chords, and text. All
// layout translation and OS event construction live in the implementation; the
// facade never touches a native API.
//
// Modifier state (critical contract): keyDown/keyUp are also how modifier keys
// (Key::LeftShift, Key::LeftMeta, ...) are pressed and released, because the
// facade builds chords by pressing physical modifier keys around the main key.
// An implementation MUST therefore track which modifier keys are currently held
// and reflect them on every subsequent synthesized event. This is not optional
// on macOS, where a character key event carries its active modifiers as CGEvent
// flags rather than deriving them from separately-posted modifier events; an
// implementation that ignores held state will produce 'c' where 'Command-c' was
// intended. On Windows and X11 the separately-injected modifier key-down is
// usually sufficient, but tracking state keeps behaviour uniform.
//
// typeUnicode is a distinct path that injects a character directly, bypassing
// keycodes and layout entirely (CGEventKeyboardSetUnicodeString, KEYEVENTF_
// UNICODE, XTest with a temporarily remapped keysym). It is the only correct way
// to emit arbitrary characters and must not be emulated with keyDown/keyUp.
class IKeyboardBackend {
 public:
  virtual ~IKeyboardBackend() = default;

  // Press or release a physical key identified by position. Unmappable keys on
  // this platform return ErrorCode::UnmappableInput.
  [[nodiscard]] virtual std::expected<void, Error> keyDown(Key key) = 0;
  [[nodiscard]] virtual std::expected<void, Error> keyUp(Key key) = 0;

  // Inject one Unicode scalar value as text, independent of keyboard layout.
  // Returns ErrorCode::Unsupported if the backend cannot inject Unicode.
  [[nodiscard]] virtual std::expected<void, Error> typeUnicode(
      char32_t codepoint
  ) = 0;
};

}  // namespace robot::backend
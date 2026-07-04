#pragma once

#include <X11/Xlib.h>

#include "X11Display.h"
#include "robot/backend/IKeyboardBackend.h"

namespace robot::x11 {

// XTest keyboard injection. Physical keys resolve to the keycode the server
// currently binds to the key's keysym (position-correct for Latin layouts).
// typeUnicode injects an arbitrary character by temporarily binding it to a
// spare keycode, faking the keypress, then restoring the mapping - the standard
// XTest technique for emitting characters that have no key on the active layout.
// This makes text output layout-independent even though X itself is keycode-based.
//
// Holds the shared connection by reference; does not own it.
class X11KeyboardBackend final : public backend::IKeyboardBackend {
 public:
  explicit X11KeyboardBackend(const X11Connection& connection)
      : connection_(&connection) {}

  std::expected<void, Error> keyDown(Key key) override;
  std::expected<void, Error> keyUp(Key key) override;
  std::expected<void, Error> typeUnicode(char32_t codepoint) override;

 private:
  std::expected<KeyCode, Error> resolveKeycode(Key key);

  const X11Connection* connection_;
};

}  // namespace robot::x11
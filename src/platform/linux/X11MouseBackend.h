#pragma once

#include "X11Display.h"
#include "robot/backend/IMouseBackend.h"

namespace robot::x11 {

// XTest mouse injection. X11 global coordinates are physical pixels; RandR
// reports per-monitor bounds so the screen backend derives logical/physical
// scaling, but XTest positioning itself is in server pixels. Buttons 1/2/3 are
// left/middle/right; 4-7 are wheel up/down/left/right; 8/9 are X1/X2. XTest keeps
// the pointer button state, so a move while a button is held is already a drag.
class X11MouseBackend final : public backend::IMouseBackend {
 public:
  explicit X11MouseBackend(const X11Connection& connection)
      : connection_(&connection) {}

  std::expected<void, Error> warpCursor(LogicalPoint point) override;
  std::expected<LogicalPoint, Error> cursorPosition() override;
  std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  ) override;
  std::expected<void, Error> scroll(ScrollDelta delta) override;

 private:
  const X11Connection* connection_;
};

}  // namespace robot::x11
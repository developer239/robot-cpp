#pragma once

#include <expected>

#include "robot/Error.h"
#include "robot/Geometry.h"
#include "robot/MouseButton.h"
#include "robot/Scroll.h"

namespace robot::backend {

// Native mouse injection in global logical coordinates. Atomic operations only;
// the portable Mouse facade builds smooth moves, drags, clicks, and double
// clicks on top of these.
//
// Button-drag contract (critical): warpCursor must emit a drag event rather than
// a plain move whenever a mouse button is currently held, or drags will not
// register in target applications (this is a hard requirement on macOS, where
// kCGEventLeftMouseDragged and kCGEventMouseMoved are different event types). An
// implementation therefore tracks which button, if any, is pressed - as
// per-instance state owned by the session, never a global. Button and scroll
// events act at the current cursor position, so callers warp first, then act.
class IMouseBackend {
 public:
  virtual ~IMouseBackend() = default;

  // Absolute positioning on the virtual desktop. Backends that cannot warp the
  // cursor (unprivileged Wayland) return ErrorCode::Unsupported.
  [[nodiscard]] virtual std::expected<void, Error> warpCursor(
      LogicalPoint point
  ) = 0;

  // Current pointer position. ErrorCode::Unsupported where it cannot be read.
  [[nodiscard]] virtual std::expected<LogicalPoint, Error> cursorPosition() = 0;

  // Press or release a button at the current position. clickCount is reported to
  // the OS so native multi-click detection can fire: pass 1 for a single click,
  // 2 for the second down/up of a double click. X1/X2 return
  // ErrorCode::Unsupported where extra buttons cannot be injected.
  [[nodiscard]] virtual std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  ) = 0;

  // Scroll at the current position. Pixel-unit deltas return
  // ErrorCode::Unsupported where high-resolution scrolling is unavailable,
  // rather than silently quantizing to lines.
  [[nodiscard]] virtual std::expected<void, Error> scroll(
      ScrollDelta delta
  ) = 0;
};

}  // namespace robot::backend
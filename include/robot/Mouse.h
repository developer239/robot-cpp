#pragma once

#include <chrono>
#include <expected>

#include "robot/Error.h"
#include "robot/Geometry.h"
#include "robot/MouseButton.h"
#include "robot/Scroll.h"

namespace robot {
namespace backend {
class IMouseBackend;
}

// Parameters for an interpolated move. The path is sampled at a fixed number of
// steps over a fixed duration, so a given move is reproducible and its timing is
// predictable (no speed-derived jitter). steps == 0 derives a step count from
// the pixel distance, capped internally, so short hops are cheap and long
// sweeps stay smooth.
struct MouseMoveOptions {
  std::chrono::milliseconds duration{300};
  int steps = 0;
};

// Mouse control operating in global logical coordinates (the virtual desktop
// space shared by all monitors). Absolute positioning and reads require the
// corresponding capabilities; under a backend that lacks them (for example
// Wayland) move()/position() return ErrorCode::Unsupported instead of silently
// doing nothing.
//
// High-level gestures (moveSmooth, drag, dragSmooth) are implemented portably by
// composing atomic backend operations with timed delays, so they behave
// identically on every platform and can be tested against a mock backend.
//
// Obtained from Session::mouse(); holds a non-owning reference to the backend.
class Mouse {
 public:
  explicit Mouse(backend::IMouseBackend& backend) : backend_(&backend) {}

  // Absolute warp to a logical point on the virtual desktop.
  [[nodiscard]] std::expected<void, Error> move(LogicalPoint point);

  // Interpolated move from the current position to point (see MouseMoveOptions).
  [[nodiscard]] std::expected<void, Error> moveSmooth(
      LogicalPoint point, const MouseMoveOptions& options = {}
  );

  [[nodiscard]] std::expected<LogicalPoint, Error> position();

  [[nodiscard]] std::expected<void, Error> press(MouseButton button);
  [[nodiscard]] std::expected<void, Error> release(MouseButton button);

  // Raw button transition with an explicit click count. This is the atom used by
  // higher-level language bindings that compose multi-click gestures themselves.
  [[nodiscard]] std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  );

  // Press then release at the current position. X1/X2 require
  // supportsExtraMouseButtons or the call fails with Unsupported.
  [[nodiscard]] std::expected<void, Error> click(
      MouseButton button = MouseButton::Left
  );

  // Two clicks reported to the OS with clickCount == 2 so native double-click
  // detection fires, rather than two unrelated single clicks.
  [[nodiscard]] std::expected<void, Error> doubleClick(
      MouseButton button = MouseButton::Left
  );

  // Press at the current position, warp to target, release. dragSmooth
  // interpolates the movement while the button is held.
  [[nodiscard]] std::expected<void, Error> drag(
      LogicalPoint to, MouseButton button = MouseButton::Left
  );
  [[nodiscard]] std::expected<void, Error> dragSmooth(
      LogicalPoint to, MouseButton button = MouseButton::Left,
      const MouseMoveOptions& options = {}
  );

  // Scroll at the current position. Pixel-unit deltas require
  // supportsHighResolutionScroll; otherwise the call fails rather than rounding
  // pixels to lines behind the caller's back.
  [[nodiscard]] std::expected<void, Error> scroll(ScrollDelta delta);

 private:
  backend::IMouseBackend* backend_;
};

}  // namespace robot

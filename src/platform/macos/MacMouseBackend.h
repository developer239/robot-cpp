#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <optional>

#include "MacKeyboardBackend.h"
#include "robot/backend/IMouseBackend.h"

namespace robot::mac {

// Quartz mouse injection. macOS global event coordinates are in logical points
// with a top-left origin, so LogicalPoint maps directly to CGPoint. Tracks the
// currently-pressed button (per instance, not global) so warpCursor emits a drag
// event while a button is held - the requirement that makes drags register.
// Every event carries the modifiers the keyboard backend holds.
class MacMouseBackend final : public backend::IMouseBackend {
 public:
  explicit MacMouseBackend(const MacKeyboardBackend& keyboard);
  ~MacMouseBackend() override;

  MacMouseBackend(const MacMouseBackend&) = delete;
  MacMouseBackend& operator=(const MacMouseBackend&) = delete;

  std::expected<void, Error> warpCursor(LogicalPoint point) override;
  std::expected<void, Error> moveRelative(
      double deltaX, double deltaY
  ) override;
  std::expected<LogicalPoint, Error> cursorPosition() override;
  std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  ) override;
  std::expected<void, Error> scroll(ScrollDelta delta) override;

 private:
  void post(CGEventRef event) const;

  const MacKeyboardBackend& keyboard_;
  CGEventSourceRef source_ = nullptr;
  std::optional<MouseButton> pressedButton_;
};

}  // namespace robot::mac
#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <optional>

#include "robot/backend/IMouseBackend.h"

namespace robot::mac {

// Quartz mouse injection. macOS global event coordinates are in logical points
// with a top-left origin, so LogicalPoint maps directly to CGPoint. Tracks the
// currently-pressed button (per instance, not global) so warpCursor emits a drag
// event while a button is held - the requirement that makes drags register.
class MacMouseBackend final : public backend::IMouseBackend {
 public:
  MacMouseBackend();
  ~MacMouseBackend() override;

  MacMouseBackend(const MacMouseBackend&) = delete;
  MacMouseBackend& operator=(const MacMouseBackend&) = delete;

  std::expected<void, Error> warpCursor(LogicalPoint point) override;
  std::expected<LogicalPoint, Error> cursorPosition() override;
  std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  ) override;
  std::expected<void, Error> scroll(ScrollDelta delta) override;

 private:
  CGEventSourceRef source_ = nullptr;
  std::optional<MouseButton> pressedButton_;
};

}  // namespace robot::mac
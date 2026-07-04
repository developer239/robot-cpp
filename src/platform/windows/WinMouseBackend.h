#pragma once

#include <Windows.h>

#include <optional>

#include "robot/backend/IMouseBackend.h"

namespace robot::win {

// SendInput mouse injection across the full virtual desktop. Absolute moves use
// MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK, which requires coordinates
// normalized to the 0..65535 range over the virtual-screen rectangle - so a
// logical point is converted against the virtual-desktop origin and extent
// (SM_XVIRTUALSCREEN / SM_CXVIRTUALSCREEN), giving correct multi-monitor
// positioning including monitors at negative coordinates. X1/X2 are injected via
// MOUSEEVENTF_XDOWN/UP with the XBUTTON1/2 data field.
//
// Unlike macOS, Windows keeps a live button state internally, so a plain
// absolute move while a button is held is already delivered as a drag; the
// pressedButton_ field is retained only to keep the cross-platform contract
// uniform, not because a separate drag event must be synthesized.
class WinMouseBackend final : public backend::IMouseBackend {
 public:
  std::expected<void, Error> warpCursor(LogicalPoint point) override;
  std::expected<LogicalPoint, Error> cursorPosition() override;
  std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  ) override;
  std::expected<void, Error> scroll(ScrollDelta delta) override;

 private:
  std::optional<MouseButton> pressedButton_;
};

}  // namespace robot::win
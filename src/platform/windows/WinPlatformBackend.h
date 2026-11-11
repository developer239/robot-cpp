#pragma once

#include <memory>

#include "WinEventTapBackend.h"
#include "WinKeyboardBackend.h"
#include "WinMouseBackend.h"
#include "WinScreenBackend.h"
#include "WinWindowBackend.h"
#include "robot/backend/IPlatformBackend.h"

namespace robot::win {

// Owns the five Windows sub-backends and the capabilities report. A Session owns
// exactly one of these.
class WinPlatformBackend final : public backend::IPlatformBackend {
 public:
  explicit WinPlatformBackend(Capabilities capabilities)
      : capabilities_(std::move(capabilities)),
        keyboard_(std::make_unique<WinKeyboardBackend>()),
        mouse_(std::make_unique<WinMouseBackend>()),
        screen_(std::make_unique<WinScreenBackend>()),
        eventTap_(std::make_unique<WinEventTapBackend>()),
        windows_(std::make_unique<WinWindowBackend>()) {}

  backend::IKeyboardBackend& keyboard() override { return *keyboard_; }
  backend::IMouseBackend& mouse() override { return *mouse_; }
  backend::IScreenBackend& screen() override { return *screen_; }
  backend::IEventTapBackend* eventTap() override { return eventTap_.get(); }
  backend::IWindowBackend* windows() override { return windows_.get(); }
  const Capabilities& capabilities() const override { return capabilities_; }

 private:
  Capabilities capabilities_;
  std::unique_ptr<WinKeyboardBackend> keyboard_;
  std::unique_ptr<WinMouseBackend> mouse_;
  std::unique_ptr<WinScreenBackend> screen_;
  std::unique_ptr<WinEventTapBackend> eventTap_;
  std::unique_ptr<WinWindowBackend> windows_;
};

}  // namespace robot::win
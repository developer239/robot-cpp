#pragma once

#include <memory>

#include "MacEventTapBackend.h"
#include "MacKeyboardBackend.h"
#include "MacMouseBackend.h"
#include "MacScreenBackend.h"
#include "robot/backend/IPlatformBackend.h"

namespace robot::mac {

// Owns the four macOS sub-backends and the capabilities report assembled at
// creation (after permission preflight). A Session owns exactly one of these.
class MacPlatformBackend final : public backend::IPlatformBackend {
 public:
  explicit MacPlatformBackend(Capabilities capabilities)
      : capabilities_(std::move(capabilities)),
        keyboard_(std::make_unique<MacKeyboardBackend>()),
        mouse_(std::make_unique<MacMouseBackend>()),
        screen_(std::make_unique<MacScreenBackend>()),
        eventTap_(std::make_unique<MacEventTapBackend>()) {}

  backend::IKeyboardBackend& keyboard() override { return *keyboard_; }
  backend::IMouseBackend& mouse() override { return *mouse_; }
  backend::IScreenBackend& screen() override { return *screen_; }
  backend::IEventTapBackend* eventTap() override { return eventTap_.get(); }
  const Capabilities& capabilities() const override { return capabilities_; }

 private:
  Capabilities capabilities_;
  std::unique_ptr<MacKeyboardBackend> keyboard_;
  std::unique_ptr<MacMouseBackend> mouse_;
  std::unique_ptr<MacScreenBackend> screen_;
  std::unique_ptr<MacEventTapBackend> eventTap_;
};

}  // namespace robot::mac
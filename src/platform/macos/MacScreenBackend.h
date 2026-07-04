#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <utility>
#include <vector>

#include "robot/backend/IScreenBackend.h"

namespace robot::mac {

// Quartz display enumeration and capture. Capture is density-correct: an exact
// monitor region is captured per-display at that display's native resolution
// (works regardless of each display's scale factor), and an arbitrary virtual-
// desktop region is captured via the global window-list path. In both cases the
// returned Image is sized to the real pixel dimensions of the produced CGImage,
// not to a point-based guess - this is the fix for the old Retina bug where the
// buffer size and the reported screen size disagreed by the backing scale.
class MacScreenBackend final : public backend::IScreenBackend {
 public:
  std::expected<std::vector<Monitor>, Error> enumerateMonitors() override;
  std::expected<Image, Error> captureRegion(PhysicalRect region) override;

 private:
  // Displays paired with their CGDirectDisplayID, used both to report monitors
  // and to route an exact-region capture to the owning display.
  std::vector<std::pair<CGDirectDisplayID, Monitor>> enumerate();
};

}  // namespace robot::mac
#pragma once

#include "X11Display.h"
#include "robot/backend/IScreenBackend.h"

namespace robot::x11 {

// XGetImage capture with RandR monitor enumeration. RandR reports each output's
// CRTC geometry, so monitors are enumerated with real per-output bounds including
// negative origins for displays left of or above the primary. X11 has no
// per-monitor logical scaling at the core level (the desktop is one pixel grid),
// so scaleFactor is reported as 1.0 and logical bounds equal physical bounds - an
// honest statement of what X exposes rather than a guessed HiDPI factor. Capture
// converts the server image's pixel layout into canonical straight RGBA.
class X11ScreenBackend final : public backend::IScreenBackend {
 public:
  explicit X11ScreenBackend(const X11Connection& connection)
      : connection_(&connection) {}

  std::expected<std::vector<Monitor>, Error> enumerateMonitors() override;
  std::expected<Image, Error> captureRegion(PhysicalRect region) override;

 private:
  const X11Connection* connection_;
};

}  // namespace robot::x11
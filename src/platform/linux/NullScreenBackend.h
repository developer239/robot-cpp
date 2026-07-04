#pragma once

#include "robot/backend/IScreenBackend.h"

namespace robot::linux_backend {

// Screen backend for the uinput assembly, which has no capture or enumeration
// path. Rather than fabricate monitors or blank images, both operations return a
// specific Unsupported error so the limitation is explicit at the call site and
// consistent with the capability flags.
class NullScreenBackend final : public backend::IScreenBackend {
 public:
  std::expected<std::vector<Monitor>, Error> enumerateMonitors() override {
    return std::unexpected(Error::unsupported(
        "monitor enumeration is unavailable through the uinput backend; use the "
        "X11 backend for screen access"
    ));
  }
  std::expected<Image, Error> captureRegion(PhysicalRect /*region*/) override {
    return std::unexpected(Error::unsupported(
        "screen capture is unavailable through the uinput backend; use the X11 "
        "backend for screen access"
    ));
  }
};

}  // namespace robot::linux_backend
#pragma once

#include <expected>
#include <vector>

#include "robot/Error.h"
#include "robot/Geometry.h"
#include "robot/Image.h"
#include "robot/Monitor.h"

namespace robot::backend {

// Native display enumeration and capture. Kept to two operations; the Screen
// facade derives primary-monitor lookup, virtual-desktop bounds, per-monitor
// capture, and single-pixel sampling from these portably, so that derived logic
// is tested once against a mock rather than reimplemented per platform.
//
// The implementation is responsible for the density-correct facts every backend
// historically got wrong: enumerateMonitors must fill both logicalBounds and
// physicalBounds and the per-display scaleFactor (a Retina display reports 2x,
// so its physical size is twice its logical size), and captureRegion must return
// an Image whose pixel dimensions equal the requested device-pixel region -
// never a buffer sized in logical points while described in pixels.
class IScreenBackend {
 public:
  virtual ~IScreenBackend() = default;

  // All displays, primary first, each with both coordinate spaces and its own
  // scale factor. ErrorCode::Unsupported if enumeration is unavailable.
  [[nodiscard]] virtual std::expected<std::vector<Monitor>, Error>
  enumerateMonitors() = 0;

  // Capture a device-pixel rectangle from the virtual desktop. The returned
  // Image is dense RGBA sized exactly to region.size. Permission gates surface as
  // ErrorCode::PermissionDenied (macOS Screen Recording); other failures as
  // ErrorCode::CaptureFailed.
  [[nodiscard]] virtual std::expected<Image, Error> captureRegion(
      PhysicalRect region
  ) = 0;
};

}  // namespace robot::backend
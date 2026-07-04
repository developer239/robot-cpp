#pragma once

#include <cstdint>
#include <expected>
#include <vector>

#include "robot/Capabilities.h"
#include "robot/Error.h"
#include "robot/Geometry.h"
#include "robot/Image.h"
#include "robot/Monitor.h"

namespace robot {
namespace backend {
class IScreenBackend;
}

// Screen enumeration and capture. Capture regions are specified in device pixels
// (PhysicalRect) because that is the only unambiguous unit for pixel data across
// mixed-density displays; use a Monitor's physicalBounds, or captureMonitor, to
// avoid doing the scale math by hand. Every operation requires the relevant
// capability and fails loudly (Unsupported / PermissionDenied / CaptureFailed)
// when the environment cannot satisfy it.
//
// Obtained from Session::screen(); holds a non-owning reference to the backend.
class Screen {
 public:
  explicit Screen(backend::IScreenBackend& backend) : backend_(&backend) {}

  // All connected displays, primary first. Requires canEnumerateMonitors.
  [[nodiscard]] std::expected<std::vector<Monitor>, Error> monitors();

  [[nodiscard]] std::expected<Monitor, Error> primaryMonitor();

  // The bounding box of the whole virtual desktop in device pixels: the union of
  // every monitor's physicalBounds, accounting for negative origins.
  [[nodiscard]] std::expected<PhysicalRect, Error> virtualBounds();

  // Capture an arbitrary device-pixel rectangle from the virtual desktop.
  [[nodiscard]] std::expected<Image, Error> capture(PhysicalRect region);

  // Capture a single monitor at its native pixel resolution.
  [[nodiscard]] std::expected<Image, Error> captureMonitor(
      std::uint32_t monitorId
  );

  // Sample one pixel at a device-pixel coordinate on the virtual desktop.
  [[nodiscard]] std::expected<Rgba, Error> pixel(PhysicalPoint point);

 private:
  backend::IScreenBackend* backend_;
};

}  // namespace robot
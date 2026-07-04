#include "robot/Screen.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <limits>

#include "robot/backend/IScreenBackend.h"

namespace robot {

std::expected<std::vector<Monitor>, Error> Screen::monitors() {
  auto monitors = backend_->enumerateMonitors();
  if (!monitors) return std::unexpected(monitors.error());
  // Guarantee the documented "primary first" order regardless of backend quirks,
  // stably so the remaining order is preserved.
  std::stable_partition(
      monitors->begin(), monitors->end(),
      [](const Monitor& m) { return m.isPrimary; }
  );
  return monitors;
}

std::expected<Monitor, Error> Screen::primaryMonitor() {
  auto monitors = backend_->enumerateMonitors();
  if (!monitors) return std::unexpected(monitors.error());
  if (monitors->empty()) {
    return std::unexpected(Error::platformError("no monitors reported"));
  }
  for (const Monitor& m : *monitors) {
    if (m.isPrimary) return m;
  }
  return monitors->front();
}

std::expected<PhysicalRect, Error> Screen::virtualBounds() {
  auto monitors = backend_->enumerateMonitors();
  if (!monitors) return std::unexpected(monitors.error());
  if (monitors->empty()) {
    return std::unexpected(Error::platformError("no monitors reported"));
  }

  std::int32_t minX = std::numeric_limits<std::int32_t>::max();
  std::int32_t minY = std::numeric_limits<std::int32_t>::max();
  std::int32_t maxX = std::numeric_limits<std::int32_t>::min();
  std::int32_t maxY = std::numeric_limits<std::int32_t>::min();

  for (const Monitor& m : *monitors) {
    minX = std::min(minX, m.physicalBounds.left());
    minY = std::min(minY, m.physicalBounds.top());
    maxX = std::max(maxX, m.physicalBounds.right());
    maxY = std::max(maxY, m.physicalBounds.bottom());
  }

  return PhysicalRect{.origin = {minX, minY},
                      .size = {maxX - minX, maxY - minY}};
}

std::expected<Image, Error> Screen::capture(const PhysicalRect region) {
  if (region.size.width <= 0 || region.size.height <= 0) {
    return std::unexpected(
        Error::invalidArgument("capture region must have positive size")
    );
  }
  return backend_->captureRegion(region);
}

std::expected<Image, Error> Screen::captureMonitor(
    const std::uint32_t monitorId
) {
  auto monitors = backend_->enumerateMonitors();
  if (!monitors) return std::unexpected(monitors.error());
  for (const Monitor& m : *monitors) {
    if (m.id == monitorId) return backend_->captureRegion(m.physicalBounds);
  }
  return std::unexpected(Error::monitorNotFound(monitorId));
}

std::expected<Rgba, Error> Screen::pixel(const PhysicalPoint point) {
  const PhysicalRect region{.origin = point, .size = {1, 1}};
  auto image = backend_->captureRegion(region);
  if (!image) return std::unexpected(image.error());
  return image->at(0, 0);
}

}  // namespace robot
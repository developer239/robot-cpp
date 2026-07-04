#pragma once

#include <cmath>
#include <cstdint>

namespace robot {

// Coordinate spaces are kept in distinct types so a value in device pixels can
// never be passed where a logical (DPI-independent) value is expected, and the
// reverse. Conversions are always explicit and always go through a scale factor
// (per-display factors live on Monitor).
//
//   Logical*  : DPI-independent desktop units (macOS "points", Windows DIPs).
//               Cursor position and movement operate here so behaviour is the
//               same across displays of differing pixel density.
//   Physical* : device pixels. Screen capture, pixel access, and image sizes
//               live here.

struct LogicalPoint {
  double x = 0.0;
  double y = 0.0;

  [[nodiscard]] double distanceTo(const LogicalPoint other) const {
    const double dx = other.x - x;
    const double dy = other.y - y;
    return std::sqrt(dx * dx + dy * dy);
  }

  friend bool operator==(LogicalPoint, LogicalPoint) = default;
};

struct PhysicalPoint {
  std::int32_t x = 0;
  std::int32_t y = 0;

  friend bool operator==(PhysicalPoint, PhysicalPoint) = default;
};

struct LogicalSize {
  double width = 0.0;
  double height = 0.0;

  friend bool operator==(LogicalSize, LogicalSize) = default;
};

struct PhysicalSize {
  std::int32_t width = 0;
  std::int32_t height = 0;

  [[nodiscard]] std::int64_t area() const {
    return static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height);
  }

  friend bool operator==(PhysicalSize, PhysicalSize) = default;
};

struct LogicalRect {
  LogicalPoint origin;
  LogicalSize size;

  [[nodiscard]] double left() const { return origin.x; }
  [[nodiscard]] double top() const { return origin.y; }
  [[nodiscard]] double right() const { return origin.x + size.width; }
  [[nodiscard]] double bottom() const { return origin.y + size.height; }

  [[nodiscard]] bool contains(const LogicalPoint p) const {
    return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
  }

  friend bool operator==(LogicalRect, LogicalRect) = default;
};

struct PhysicalRect {
  PhysicalPoint origin;
  PhysicalSize size;

  [[nodiscard]] std::int32_t left() const { return origin.x; }
  [[nodiscard]] std::int32_t top() const { return origin.y; }
  [[nodiscard]] std::int32_t right() const { return origin.x + size.width; }
  [[nodiscard]] std::int32_t bottom() const { return origin.y + size.height; }

  [[nodiscard]] bool contains(const PhysicalPoint p) const {
    return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
  }

  friend bool operator==(PhysicalRect, PhysicalRect) = default;
};

// scaleFactor is physical pixels per logical unit: 2.0 on a typical Retina
// display, 1.5 for a 150% Windows display, 1.0 for a standard-density display.
[[nodiscard]] inline PhysicalPoint toPhysical(
    const LogicalPoint p, const double scaleFactor
) {
  return {static_cast<std::int32_t>(std::lround(p.x * scaleFactor)),
          static_cast<std::int32_t>(std::lround(p.y * scaleFactor))};
}

[[nodiscard]] inline LogicalPoint toLogical(
    const PhysicalPoint p, const double scaleFactor
) {
  return {static_cast<double>(p.x) / scaleFactor,
          static_cast<double>(p.y) / scaleFactor};
}

[[nodiscard]] inline PhysicalSize toPhysical(
    const LogicalSize s, const double scaleFactor
) {
  return {static_cast<std::int32_t>(std::lround(s.width * scaleFactor)),
          static_cast<std::int32_t>(std::lround(s.height * scaleFactor))};
}

[[nodiscard]] inline LogicalSize toLogical(
    const PhysicalSize s, const double scaleFactor
) {
  return {static_cast<double>(s.width) / scaleFactor,
          static_cast<double>(s.height) / scaleFactor};
}

}  // namespace robot

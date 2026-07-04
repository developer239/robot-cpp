#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <utility>
#include <vector>

#include "robot/Error.h"
#include "robot/Geometry.h"

namespace robot {

// One pixel in straight (non-premultiplied) RGBA, 8 bits per channel. Backends
// normalize their native pixel order (BGRA on Windows GDI, premultiplied ARGB
// from Quartz) into this canonical layout before returning an Image, so callers
// never deal with platform byte order.
struct Rgba {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;

  friend bool operator==(Rgba, Rgba) = default;
};

// A captured region as a dense row-major RGBA buffer. Dimensions are in device
// pixels (PhysicalSize): a 100x100 logical capture on a 2x display yields a
// 200x200 Image, and that pixel count is the truth about what was captured.
// Value type with move semantics; the pixel buffer is owned.
class Image {
 public:
  Image() = default;
  Image(const PhysicalSize size, std::vector<Rgba> pixels)
      : size_(size), pixels_(std::move(pixels)) {}

  [[nodiscard]] PhysicalSize size() const { return size_; }
  [[nodiscard]] std::int32_t width() const { return size_.width; }
  [[nodiscard]] std::int32_t height() const { return size_.height; }
  [[nodiscard]] bool empty() const { return pixels_.empty(); }

  [[nodiscard]] const std::vector<Rgba>& pixels() const { return pixels_; }

  // Bounds-checked pixel access. Out-of-range coordinates are a programming
  // error surfaced as InvalidArgument rather than undefined behaviour.
  [[nodiscard]] std::expected<Rgba, Error> at(
      std::int32_t x, std::int32_t y
  ) const;

  // Encode as PNG and write to disk. Defined in the common layer against the
  // vendored PNG encoder, which stays a private dependency (no encoder headers
  // leak into this public header).
  [[nodiscard]] std::expected<void, Error> savePng(
      std::string_view path
  ) const;

 private:
  PhysicalSize size_;
  std::vector<Rgba> pixels_;
};

}  // namespace robot
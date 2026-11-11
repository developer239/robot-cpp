#pragma once

// Typed Windows bridge for VideoFrame's platform buffer. Include this from
// Windows-only translation units instead of casting nativeHandle() by hand.

#include <cstdint>

#include "robot/VideoFrame.h"

namespace robot::win {

// Frame pixels mapped for CPU reads, BGRA and top-down. Valid for the frame's
// lifetime, and safe to read from any thread.
struct PixelBuffer {
  const std::uint8_t* pixels = nullptr;
  int width = 0;
  int height = 0;
  int bytesPerRow = 0;
};

[[nodiscard]] inline const PixelBuffer& pixelBuffer(const VideoFrame& frame) {
  return *static_cast<const PixelBuffer*>(frame.nativeHandle());
}

}  // namespace robot::win

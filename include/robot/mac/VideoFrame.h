#pragma once

// Typed macOS bridge for VideoFrame's platform buffer. Include this from
// macOS-only translation units instead of casting nativeHandle() by hand.

#include <CoreVideo/CoreVideo.h>

#include "robot/VideoFrame.h"

namespace robot::mac {

// The frame's pixel buffer, borrowed for the frame's lifetime (the VideoFrame
// keeps its retain). BGRA layout; lock via CVPixelBufferLockBaseAddress for
// CPU access.
[[nodiscard]] inline CVPixelBufferRef cvPixelBuffer(const VideoFrame& frame) {
  return static_cast<CVPixelBufferRef>(frame.nativeHandle());
}

}  // namespace robot::mac

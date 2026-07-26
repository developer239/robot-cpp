#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "robot/Geometry.h"

namespace robot {

// Per-frame metadata for a streamed window frame. The scale fields and the
// content rectangle map buffer pixels back to window points - see
// toWindowPoint() below, which is the one implementation of that formula.
// All fields are zero when the OS omitted the metadata; consumers must refuse
// coordinate-dependent work rather than guess.
struct VideoFrameInfo {
  std::uint32_t windowId = 0;
  PhysicalSize size;          // buffer size in device pixels
  double scaleFactor = 0.0;   // device pixels per surface point (Retina: 2.0)
  double contentScale = 0.0;  // downscale applied to fit content in the buffer
  // Where the window content sits within the buffer's surface, in surface
  // points. The content can sit away from the buffer origin (shadows,
  // padding, resize letterboxing), so this is part of the contract.
  LogicalRect contentRect;
  // Presentation timestamp mapped onto steady_clock (not delivery time), so
  // frame age reflects how old the pixels actually are.
  std::chrono::steady_clock::time_point capturedAt;
};

// Maps a buffer pixel coordinate to window-local logical coordinates:
//
//   surfacePoint = pixel / scaleFactor
//   windowPoint  = (surfacePoint - contentRect.origin) / contentScale
//
// Returns nullopt when the metadata is missing or the pixel lies outside the
// window content area (shadow/letterbox padding) - callers must treat that as
// a refusal, not round it into range.
[[nodiscard]] inline std::optional<LogicalPoint> toWindowPoint(
    const VideoFrameInfo& info, const double pixelX, const double pixelY
) {
  if (info.scaleFactor <= 0.0 || info.contentScale <= 0.0 ||
      info.contentRect.size.width <= 0.0 ||
      info.contentRect.size.height <= 0.0) {
    return std::nullopt;
  }

  const double contentX = pixelX / info.scaleFactor - info.contentRect.origin.x;
  const double contentY = pixelY / info.scaleFactor - info.contentRect.origin.y;
  if (contentX < 0.0 || contentX >= info.contentRect.size.width ||
      contentY < 0.0 || contentY >= info.contentRect.size.height) {
    return std::nullopt;
  }

  return LogicalPoint{
      .x = contentX / info.contentScale,
      .y = contentY / info.contentScale,
  };
}

// One streamed frame: metadata plus a retained, shareable reference to the
// platform pixel buffer (BGRA layout). Copies share the same buffer - a frame
// is never deep-copied on its way through a consumer's pipeline.
//
// Access the pixels through the typed platform bridge (robot/mac/VideoFrame.h
// on macOS) rather than casting nativeHandle() by hand. A portable pixel
// accessor is deliberately absent until a cross-platform consumer exists.
class VideoFrame {
 public:
  VideoFrame() = default;

  // Constructed by backends; the buffer's deleter releases the platform
  // reference the backend retained.
  VideoFrame(const VideoFrameInfo& info, std::shared_ptr<void> nativeBuffer)
      : info_(info), nativeBuffer_(std::move(nativeBuffer)) {}

  [[nodiscard]] const VideoFrameInfo& info() const noexcept { return info_; }

  // Borrowed for this frame's lifetime. Prefer the typed platform bridge.
  [[nodiscard]] void* nativeHandle() const noexcept {
    return nativeBuffer_.get();
  }

  explicit operator bool() const noexcept { return nativeBuffer_ != nullptr; }

 private:
  VideoFrameInfo info_;
  std::shared_ptr<void> nativeBuffer_;
};

}  // namespace robot

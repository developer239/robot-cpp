#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

#include "robot/Error.h"
#include "robot/Geometry.h"
#include "robot/Window.h"
#include "robot/WindowStream.h"

namespace robot::backend {

// One platform window-capture stream. Implementations own the delivery
// thread; stop() must uphold the WindowStream drain contract (no callback
// executing or starting once it returns).
class IWindowStreamBackend {
 public:
  virtual ~IWindowStreamBackend() = default;

  virtual void stop() = 0;

  [[nodiscard]] virtual bool isRunning() const = 0;
};

// Native window enumeration, geometry/focus queries, and push-based window
// capture. Optional per platform build: IPlatformBackend::windows() returns
// nullptr where none of this is implemented, and the Windows facade surfaces
// that as ErrorCode::Unsupported per call - never a silent no-op.
//
// Streaming is push-based because that is how modern capture APIs deliver
// frames (macOS ScreenCaptureKit); the backend owns the delivery thread and
// the caller owns the returned stream.
class IWindowBackend {
 public:
  virtual ~IWindowBackend() = default;

  // On-screen normal-layer windows with titles, front to back.
  [[nodiscard]] virtual std::expected<std::vector<Window>, Error>
  enumerateWindows() = 0;

  // Current global-logical frame of a window; WindowNotFound once it is gone.
  [[nodiscard]] virtual std::expected<LogicalRect, Error> windowFrame(
      std::uint32_t windowId
  ) = 0;

  // Id of the topmost normal-layer window in z-order, nullopt when none is on
  // screen. An ordering query, not an input-focus query - but the topmost
  // normal window belongs to the active application in practice.
  [[nodiscard]] virtual std::expected<std::optional<std::uint32_t>, Error>
  topmostWindowId() = 0;

  // Brings the application owning the window to the foreground. Application
  // semantics, not window semantics: which of the app's windows ends up
  // frontmost is the OS's decision.
  [[nodiscard]] virtual std::expected<void, Error> activateApplicationForWindow(
      std::uint32_t windowId
  ) = 0;

  // Starts streaming the window. Frames arrive serialized on an internal
  // capture thread through onFrame; onError fires (same thread, same
  // serialization) when the stream dies outside stop().
  [[nodiscard]] virtual std::expected<
      std::unique_ptr<IWindowStreamBackend>, Error>
  startStream(
      std::uint32_t windowId, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError
  ) = 0;
};

}  // namespace robot::backend

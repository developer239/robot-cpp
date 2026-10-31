#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

#include "robot/Error.h"
#include "robot/Geometry.h"
#include "robot/Window.h"
#include "robot/WindowStream.h"

namespace robot {
namespace backend {
class IWindowBackend;
}

// Window enumeration, geometry/ordering queries, and push-based window
// capture. On platform builds without window support every call returns
// ErrorCode::Unsupported. list() and stream() require canEnumerateWindows /
// canStreamWindows; frame(), topmostWindowId(), and activateApplicationFor()
// work without special permissions on macOS.
//
// The query methods are safe to call from any thread. stream() and each
// returned WindowStream must be driven from one thread at a time.
//
// Obtained from Session::windows(); holds a non-owning pointer to the backend
// (nullptr when the platform build provides no window support).
class Windows {
 public:
  explicit Windows(backend::IWindowBackend* backend) : backend_(backend) {}

  // On-screen normal-layer windows with titles, front to back. On macOS this
  // triggers the Screen Recording permission prompt on first use.
  [[nodiscard]] std::expected<std::vector<Window>, Error> list();

  // Current global-logical frame of a window - re-query before coordinate
  // math, windows move. WindowNotFound once it is gone.
  [[nodiscard]] std::expected<LogicalRect, Error> frame(
      std::uint32_t windowId
  );

  // Id of the topmost normal-layer window in z-order; nullopt when none is on
  // screen. Window-level: another window of the same app being topmost does
  // not make the target window topmost.
  [[nodiscard]] std::expected<std::optional<std::uint32_t>, Error>
  topmostWindowId();

  // Brings the application owning the window to the foreground. Application
  // semantics: which of its windows ends up frontmost is the OS's decision.
  [[nodiscard]] std::expected<void, Error> activateApplicationFor(
      std::uint32_t windowId
  );

  // Starts streaming a window. Frames arrive serialized on an internal
  // capture thread (see VideoFrameCallback); the stream stops when the
  // returned handle is destroyed.
  [[nodiscard]] std::expected<WindowStream, Error> stream(
      std::uint32_t windowId, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError = nullptr
  );

  // Starts streaming a whole monitor, by Monitor::id. For an overlay, or for
  // following anything on the desktop rather than one window.
  [[nodiscard]] std::expected<WindowStream, Error> streamMonitor(
      std::uint32_t monitorId, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError = nullptr
  );

 private:
  [[nodiscard]] std::expected<void, Error> validateStream(
      const WindowStreamOptions& options, const VideoFrameCallback& onFrame
  ) const;

  backend::IWindowBackend* backend_;
};

}  // namespace robot

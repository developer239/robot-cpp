#pragma once

#include <functional>
#include <memory>

#include "robot/Error.h"
#include "robot/VideoFrame.h"

namespace robot {
namespace backend {
class IWindowStreamBackend;
}

struct WindowStreamOptions {
  double maxFps = 60.0;  // must be finite and in [1, 1000]
  bool showsCursor = true;
};

// Invoked on the stream's internal capture thread for every delivered frame,
// serialized and in order. The callback must be brief (hand the frame off;
// VideoFrame copies are cheap), must not throw, and must not call back into
// robot-cpp APIs or touch this stream's handle.
using VideoFrameCallback = std::function<void(const VideoFrame&)>;

// Invoked on the same capture thread (serialized with frame delivery) when
// the stream dies outside stop(): the window closed, the OS revoked capture,
// the display reconfigured. Fires at most once; a death that happens before a
// concurrent stop() is always reported, while stop() winning the race means a
// clean stop and no error. After it fires, isRunning() is false and no
// further frames arrive. Same rules as VideoFrameCallback.
using StreamErrorCallback = std::function<void(const Error&)>;

// A running window capture stream, returned by Windows::stream(). Move-only
// value type owning the platform stream; destruction stops it.
//
// stop() contract: when stop() returns, no callback is executing and none
// will start. That makes it safe to destroy callback-captured state right
// after stopping. Corollary: never call stop() (or destroy the stream) from
// inside one of its own callbacks - that would deadlock on the drain.
class WindowStream {
 public:
  explicit WindowStream(std::unique_ptr<backend::IWindowStreamBackend> impl);
  ~WindowStream();
  WindowStream(WindowStream&&) noexcept;
  WindowStream& operator=(WindowStream&&) noexcept;
  WindowStream(const WindowStream&) = delete;
  WindowStream& operator=(const WindowStream&) = delete;

  // Idempotent. Blocks until in-flight callbacks have drained.
  void stop();

  // False after stop() or a stream failure.
  [[nodiscard]] bool isRunning() const;

 private:
  std::unique_ptr<backend::IWindowStreamBackend> impl_;
};

}  // namespace robot

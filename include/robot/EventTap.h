#pragma once

#include <expected>
#include <functional>

#include "robot/Error.h"
#include "robot/Event.h"

namespace robot {
namespace backend {
class IEventTapBackend;
}

// Receives normalized global input events in real time. Kept as a std::function
// because a tap is a cold, one-per-session boundary where type erasure costs
// nothing meaningful.
using EventSink = std::function<void(const InputEvent&)>;

// A global input tap: observe all mouse and keyboard activity system-wide and
// forward it as normalized InputEvents (commonly into a Recorder). This is an
// inherently privileged, platform-limited capability - it needs Accessibility on
// macOS, low-level hooks on Windows, and is largely unavailable to unprivileged
// clients under Wayland - so a backend that cannot provide it makes start()
// return ErrorCode::Unsupported or PermissionDenied rather than pretending to
// record.
//
// Threading: start() blocks the calling thread running the platform event loop
// until stop() is called (from another thread or from within the sink). Run it
// on a dedicated thread if the caller needs to keep working.
//
// Obtained from Session::eventTap(); holds a non-owning, possibly-null backend
// pointer (null when the platform has no tap implementation, in which case
// start() reports Unsupported).
class EventTap {
 public:
  explicit EventTap(backend::IEventTapBackend* backend) : backend_(backend) {}

  [[nodiscard]] bool isSupported() const { return backend_ != nullptr; }

  [[nodiscard]] std::expected<void, Error> start(EventSink sink);
  void stop();
  [[nodiscard]] bool isRunning() const;

 private:
  backend::IEventTapBackend* backend_;
};

}  // namespace robot
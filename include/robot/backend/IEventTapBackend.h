#pragma once

#include <expected>

#include "robot/Error.h"
#include "robot/EventTap.h"

namespace robot::backend {

// Native global input tap. Translates OS events into normalized InputEvents and
// forwards them to the sink. A platform build that has no tap implementation
// simply exposes no IEventTapBackend (IPlatformBackend::eventTap() returns
// nullptr), and the EventTap facade then reports Unsupported; a build that has
// one but lacks permission returns PermissionDenied from start().
//
// start() blocks the calling thread on the platform run loop / message pump
// until stop() is invoked. The implementation must translate native key events
// into physical Key values (not characters) so recordings replay by position and
// are layout-independent, and must express positions in logical coordinates to
// match the injection side.
class IEventTapBackend {
 public:
  virtual ~IEventTapBackend() = default;

  // Begin tapping and block until stop(). ErrorCode::PermissionDenied when the
  // OS withholds the required access (macOS Accessibility).
  [[nodiscard]] virtual std::expected<void, Error> start(EventSink sink) = 0;

  // Signal the run loop to exit; safe to call from another thread or from inside
  // the sink. Idempotent.
  virtual void stop() = 0;

  [[nodiscard]] virtual bool isRunning() const = 0;
};

}  // namespace robot::backend
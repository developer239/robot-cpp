#include "robot/EventTap.h"

#include <utility>

#include "robot/backend/IEventTapBackend.h"

namespace robot {

std::expected<void, Error> EventTap::start(EventSink sink) {
  if (backend_ == nullptr) {
    return std::unexpected(Error::unsupported(
        "global event recording is not available in this platform build"
    ));
  }
  return backend_->start(std::move(sink));
}

void EventTap::stop() {
  if (backend_ != nullptr) backend_->stop();
}

bool EventTap::isRunning() const {
  return backend_ != nullptr && backend_->isRunning();
}

}  // namespace robot
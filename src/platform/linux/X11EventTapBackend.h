#pragma once

#include <X11/Xlib.h>

#include <atomic>

#include "robot/backend/IEventTapBackend.h"

namespace robot::x11 {

// Global input tap via the XRecord extension. XRecord delivers raw device events
// on a second connection while the normal one keeps running, which is the
// standard way to observe global input under X without grabbing it. Key events
// are translated by keysym into physical Key values, and pointer positions are
// reported as desktop pixels. Requires the XTEST/RECORD extension; start()
// reports Unsupported when the server lacks it. XRecord observes only the X input
// stream, so it captures nothing from native Wayland clients - a limitation the
// factory surfaces up front.
class X11EventTapBackend final : public backend::IEventTapBackend {
 public:
  X11EventTapBackend() = default;
  ~X11EventTapBackend() override = default;

  X11EventTapBackend(const X11EventTapBackend&) = delete;
  X11EventTapBackend& operator=(const X11EventTapBackend&) = delete;

  std::expected<void, Error> start(EventSink sink) override;
  void stop() override;
  [[nodiscard]] bool isRunning() const override { return running_.load(); }

 private:
  EventSink sink_;
  std::atomic<bool> running_{false};

  // Two dedicated connections: XRecord requires a data connection distinct from
  // the control one. Opened in start(), closed on teardown.
  Display* control_ = nullptr;
  Display* data_ = nullptr;
  void* context_ = nullptr;  // XRecordContext, kept opaque in the header.
};

}  // namespace robot::x11
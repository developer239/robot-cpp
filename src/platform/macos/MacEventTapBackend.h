#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <atomic>
#include <set>

#include "robot/Key.h"
#include "robot/backend/IEventTapBackend.h"

namespace robot::mac {

// Global input tap via CGEventTap. Translates native events into normalized
// InputEvents: key events carry a physical Key (from the CGKeyCode), modifier
// keys are recovered from kCGEventFlagsChanged (which the previous recorder
// ignored entirely, so it never captured Shift/Ctrl/Cmd), and positions are in
// logical points to match the injection side. Requires Accessibility; start()
// reports PermissionDenied when the tap cannot be created.
class MacEventTapBackend final : public backend::IEventTapBackend {
 public:
  MacEventTapBackend() = default;
  ~MacEventTapBackend() override = default;

  MacEventTapBackend(const MacEventTapBackend&) = delete;
  MacEventTapBackend& operator=(const MacEventTapBackend&) = delete;

  std::expected<void, Error> start(EventSink sink) override;
  void stop() override;
  [[nodiscard]] bool isRunning() const override { return running_.load(); }

 private:
  static CGEventRef callback(
      CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* userInfo
  );
  void handle(CGEventType type, CGEventRef event);

  EventSink sink_;
  // Modifier keys currently held, toggled on each flags-changed event so a
  // physical modifier's down/up is tracked per key without relying on flag bits
  // that cannot distinguish left from right.
  std::set<Key> heldModifiers_;

  CFMachPortRef tap_ = nullptr;
  CFRunLoopSourceRef source_ = nullptr;
  std::atomic<CFRunLoopRef> runLoop_{nullptr};
  std::atomic<bool> running_{false};
};

}  // namespace robot::mac
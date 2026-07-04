#include "MacEventTapBackend.h"

#include <cmath>

#include "MacKeyMap.h"
#include "robot/Event.h"

namespace robot::mac {
namespace {

MouseButton otherButton(const std::int64_t buttonNumber) {
  switch (buttonNumber) {
    case 2: return MouseButton::Middle;
    case 3: return MouseButton::X1;
    case 4: return MouseButton::X2;
    default: return MouseButton::Middle;
  }
}

}  // namespace

CGEventRef MacEventTapBackend::callback(
    CGEventTapProxy /*proxy*/, const CGEventType type, CGEventRef event,
    void* userInfo
) {
  auto* self = static_cast<MacEventTapBackend*>(userInfo);

  // macOS disables a tap if its callback is slow or on user input; it must be
  // re-enabled or recording silently stops. The old code never handled this.
  if (type == kCGEventTapDisabledByTimeout ||
      type == kCGEventTapDisabledByUserInput) {
    if (self->tap_ != nullptr) CGEventTapEnable(self->tap_, true);
    return event;
  }

  self->handle(type, event);
  return event;  // Listen-only tap; the event passes through unchanged.
}

void MacEventTapBackend::handle(const CGEventType type, CGEventRef event) {
  const CGPoint loc = CGEventGetLocation(event);
  const LogicalPoint position{loc.x, loc.y};

  switch (type) {
    case kCGEventKeyDown:
    case kCGEventKeyUp: {
      const auto code = static_cast<CGKeyCode>(
          CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)
      );
      const Key key = macKeycodeToKey(code);
      if (key != Key::Unknown) {
        sink_(KeyEvent{key, type == kCGEventKeyDown});
      }
      break;
    }
    case kCGEventFlagsChanged: {
      // Modifier keys report here, not as KeyDown/KeyUp. Toggle held state to
      // derive press vs release for the specific physical key that changed.
      const auto code = static_cast<CGKeyCode>(
          CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)
      );
      const Key key = macKeycodeToKey(code);
      if (key != Key::Unknown) {
        const bool nowDown = !heldModifiers_.contains(key);
        if (nowDown) {
          heldModifiers_.insert(key);
        } else {
          heldModifiers_.erase(key);
        }
        sink_(KeyEvent{key, nowDown});
      }
      break;
    }
    case kCGEventMouseMoved:
    case kCGEventLeftMouseDragged:
    case kCGEventRightMouseDragged:
    case kCGEventOtherMouseDragged:
      sink_(MouseMoveEvent{position});
      break;
    case kCGEventLeftMouseDown:
      sink_(MouseButtonEvent{MouseButton::Left, true, position});
      break;
    case kCGEventLeftMouseUp:
      sink_(MouseButtonEvent{MouseButton::Left, false, position});
      break;
    case kCGEventRightMouseDown:
      sink_(MouseButtonEvent{MouseButton::Right, true, position});
      break;
    case kCGEventRightMouseUp:
      sink_(MouseButtonEvent{MouseButton::Right, false, position});
      break;
    case kCGEventOtherMouseDown:
    case kCGEventOtherMouseUp: {
      const std::int64_t num =
          CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
      sink_(MouseButtonEvent{otherButton(num),
                             type == kCGEventOtherMouseDown, position});
      break;
    }
    case kCGEventScrollWheel: {
      const auto v = static_cast<double>(
          CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1)
      );
      const auto h = static_cast<double>(
          CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2)
      );
      sink_(ScrollEvent{ScrollDelta::lines(v, h), position});
      break;
    }
    default:
      break;
  }
}

std::expected<void, Error> MacEventTapBackend::start(EventSink sink) {
  sink_ = std::move(sink);
  heldModifiers_.clear();

  const CGEventMask mask =
      CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) |
      CGEventMaskBit(kCGEventFlagsChanged) |
      CGEventMaskBit(kCGEventMouseMoved) |
      CGEventMaskBit(kCGEventLeftMouseDown) |
      CGEventMaskBit(kCGEventLeftMouseUp) |
      CGEventMaskBit(kCGEventRightMouseDown) |
      CGEventMaskBit(kCGEventRightMouseUp) |
      CGEventMaskBit(kCGEventOtherMouseDown) |
      CGEventMaskBit(kCGEventOtherMouseUp) |
      CGEventMaskBit(kCGEventLeftMouseDragged) |
      CGEventMaskBit(kCGEventRightMouseDragged) |
      CGEventMaskBit(kCGEventOtherMouseDragged) |
      CGEventMaskBit(kCGEventScrollWheel);

  // Listen-only: observe without modifying the stream. Still requires
  // Accessibility, so a null tap means permission was withheld.
  tap_ = CGEventTapCreate(
      kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
      mask, &MacEventTapBackend::callback, this
  );
  if (tap_ == nullptr) {
    return std::unexpected(
        Error::permissionDenied("Accessibility (event recording)")
    );
  }

  source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap_, 0);
  CFRunLoopRef loop = CFRunLoopGetCurrent();
  runLoop_.store(loop);
  CFRunLoopAddSource(loop, source_, kCFRunLoopCommonModes);
  CGEventTapEnable(tap_, true);

  running_.store(true);
  CFRunLoopRun();  // Blocks until stop() calls CFRunLoopStop from another thread.

  // Teardown after the loop exits.
  running_.store(false);
  CGEventTapEnable(tap_, false);
  CFRunLoopRemoveSource(loop, source_, kCFRunLoopCommonModes);
  CFRelease(source_);
  CFRelease(tap_);
  source_ = nullptr;
  tap_ = nullptr;
  runLoop_.store(nullptr);
  return {};
}

void MacEventTapBackend::stop() {
  running_.store(false);
  if (CFRunLoopRef loop = runLoop_.load(); loop != nullptr) {
    CFRunLoopStop(loop);  // Safe to call from another thread.
  }
}

}  // namespace robot::mac
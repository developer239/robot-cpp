#include "MacMouseBackend.h"

#include <cmath>

namespace robot::mac {
namespace {

// CGMouseButton number for a library button. Left/Right/Middle have named
// constants; X1/X2 use "other" button numbers 3 and 4, which macOS injects.
CGMouseButton cgButton(const MouseButton b) {
  switch (b) {
    case MouseButton::Left: return kCGMouseButtonLeft;
    case MouseButton::Right: return kCGMouseButtonRight;
    case MouseButton::Middle: return kCGMouseButtonCenter;
    case MouseButton::X1: return static_cast<CGMouseButton>(3);
    case MouseButton::X2: return static_cast<CGMouseButton>(4);
  }
  return kCGMouseButtonLeft;
}

CGEventType downType(const MouseButton b) {
  switch (b) {
    case MouseButton::Left: return kCGEventLeftMouseDown;
    case MouseButton::Right: return kCGEventRightMouseDown;
    default: return kCGEventOtherMouseDown;
  }
}

CGEventType upType(const MouseButton b) {
  switch (b) {
    case MouseButton::Left: return kCGEventLeftMouseUp;
    case MouseButton::Right: return kCGEventRightMouseUp;
    default: return kCGEventOtherMouseUp;
  }
}

CGEventType dragType(const MouseButton b) {
  switch (b) {
    case MouseButton::Left: return kCGEventLeftMouseDragged;
    case MouseButton::Right: return kCGEventRightMouseDragged;
    default: return kCGEventOtherMouseDragged;
  }
}

}  // namespace

MacMouseBackend::MacMouseBackend() {
  source_ = CGEventSourceCreate(kCGEventSourceStatePrivate);
  if (source_ != nullptr) {
    CGEventSourceSetLocalEventsSuppressionInterval(source_, 0.0);
  }
}

MacMouseBackend::~MacMouseBackend() {
  if (source_ != nullptr) CFRelease(source_);
}

std::expected<void, Error> MacMouseBackend::warpCursor(const LogicalPoint point) {
  const CGPoint target = CGPointMake(point.x, point.y);

  // While a button is held, a move must be posted as that button's drag event or
  // the target application will not treat the gesture as a drag.
  const CGEventType type =
      pressedButton_ ? dragType(*pressedButton_) : kCGEventMouseMoved;
  const CGMouseButton btn =
      pressedButton_ ? cgButton(*pressedButton_) : kCGMouseButtonLeft;

  CGEventRef event = CGEventCreateMouseEvent(source_, type, target, btn);
  if (event == nullptr) {
    return std::unexpected(Error::platformError("CGEventCreateMouseEvent"));
  }
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  return {};
}

std::expected<LogicalPoint, Error> MacMouseBackend::cursorPosition() {
  CGEventRef event = CGEventCreate(nullptr);
  if (event == nullptr) {
    return std::unexpected(Error::platformError("CGEventCreate"));
  }
  const CGPoint p = CGEventGetLocation(event);
  CFRelease(event);
  return LogicalPoint{p.x, p.y};
}

std::expected<void, Error> MacMouseBackend::button(
    const MouseButton button, const ButtonAction action, const int clickCount
) {
  auto here = cursorPosition();
  if (!here) return std::unexpected(here.error());
  const CGPoint at = CGPointMake(here->x, here->y);

  const bool down = action == ButtonAction::Down;
  const CGEventType type = down ? downType(button) : upType(button);

  CGEventRef event =
      CGEventCreateMouseEvent(source_, type, at, cgButton(button));
  if (event == nullptr) {
    return std::unexpected(Error::platformError("CGEventCreateMouseEvent"));
  }
  // Report the multi-click count so native double-click detection fires.
  CGEventSetIntegerValueField(event, kCGMouseEventClickState, clickCount);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);

  if (down) {
    pressedButton_ = button;
  } else {
    pressedButton_.reset();
  }
  return {};
}

std::expected<void, Error> MacMouseBackend::scroll(const ScrollDelta delta) {
  const CGScrollEventUnit unit = delta.unit == ScrollUnit::Pixel
                                     ? kCGScrollEventUnitPixel
                                     : kCGScrollEventUnitLine;

  // wheel1 = vertical, wheel2 = horizontal. Sign follows the library convention
  // (vertical > 0 is up); the OS "natural scrolling" setting may invert the
  // visible direction, which is a user preference and intentionally not hidden.
  const int32_t wheel1 = static_cast<int32_t>(std::lround(delta.vertical));
  const int32_t wheel2 = static_cast<int32_t>(std::lround(delta.horizontal));

  CGEventRef event =
      CGEventCreateScrollWheelEvent(source_, unit, 2, wheel1, wheel2);
  if (event == nullptr) {
    return std::unexpected(
        Error::platformError("CGEventCreateScrollWheelEvent")
    );
  }
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  return {};
}

}  // namespace robot::mac
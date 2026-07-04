#include "X11MouseBackend.h"

#include <X11/extensions/XTest.h>

#include <cmath>

namespace robot::x11 {
namespace {

// X core button numbers. Wheel is expressed as button clicks (4/5 vertical,
// 6/7 horizontal), which is the X11 convention that toolkits understand.
unsigned int coreButton(const MouseButton b) {
  switch (b) {
    case MouseButton::Left: return 1;
    case MouseButton::Middle: return 2;
    case MouseButton::Right: return 3;
    case MouseButton::X1: return 8;
    case MouseButton::X2: return 9;
  }
  return 1;
}

}  // namespace

std::expected<void, Error> X11MouseBackend::warpCursor(const LogicalPoint point) {
  // XTest positions in server pixels; LogicalPoint here carries desktop pixels
  // (X has no separate logical space at the core-protocol level), so it maps 1:1.
  XTestFakeMotionEvent(
      connection_->display(), -1, static_cast<int>(std::lround(point.x)),
      static_cast<int>(std::lround(point.y)), CurrentTime
  );
  XFlush(connection_->display());
  return {};
}

std::expected<LogicalPoint, Error> X11MouseBackend::cursorPosition() {
  Window rootReturn = 0;
  Window childReturn = 0;
  int rootX = 0;
  int rootY = 0;
  int winX = 0;
  int winY = 0;
  unsigned int mask = 0;

  if (XQueryPointer(
          connection_->display(), connection_->root(), &rootReturn,
          &childReturn, &rootX, &rootY, &winX, &winY, &mask
      ) == False) {
    return std::unexpected(
        Error::platformError("XQueryPointer (pointer on another screen)")
    );
  }
  return LogicalPoint{static_cast<double>(rootX), static_cast<double>(rootY)};
}

std::expected<void, Error> X11MouseBackend::button(
    const MouseButton button, const ButtonAction action, int /*clickCount*/
) {
  // X derives double-click from timing/position between clicks, so the count is
  // not forwarded; the facade's paired clicks fall within the toolkit interval.
  XTestFakeButtonEvent(
      connection_->display(), coreButton(button),
      action == ButtonAction::Down ? True : False, CurrentTime
  );
  XFlush(connection_->display());
  return {};
}

std::expected<void, Error> X11MouseBackend::scroll(const ScrollDelta delta) {
  if (delta.unit == ScrollUnit::Pixel) {
    // Core X11 wheel is discrete button clicks; there is no pixel-precise wheel
    // in the core protocol. Rather than silently rounding pixels to notches, this
    // is reported as unsupported so callers know the granularity is unavailable.
    return std::unexpected(Error::unsupported(
        "pixel-precise scrolling is not available through X11 core wheel "
        "buttons; use line units"
    ));
  }

  Display* dpy = connection_->display();
  const auto emit = [&](const unsigned int btn, const int times) {
    for (int i = 0; i < times; ++i) {
      XTestFakeButtonEvent(dpy, btn, True, CurrentTime);
      XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
    }
  };

  // Vertical > 0 is up (button 4), < 0 is down (button 5). Horizontal > 0 is
  // right (button 7), < 0 is left (button 6).
  const int v = static_cast<int>(std::lround(std::abs(delta.vertical)));
  const int h = static_cast<int>(std::lround(std::abs(delta.horizontal)));
  if (delta.vertical > 0) emit(4, v);
  if (delta.vertical < 0) emit(5, v);
  if (delta.horizontal > 0) emit(7, h);
  if (delta.horizontal < 0) emit(6, h);

  XFlush(dpy);
  return {};
}

}  // namespace robot::x11
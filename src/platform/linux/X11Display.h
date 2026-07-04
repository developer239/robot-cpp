#pragma once

#include <X11/Xlib.h>

#include <expected>

#include "robot/Error.h"

namespace robot::x11 {

// A shared, reference-counted-by-ownership connection to the X server. One
// Display* is opened per session and handed to each X11 sub-backend by reference;
// none of them owns it. Opening fails cleanly with a specific Error when no X
// server is reachable (for example a pure Wayland session with no Xwayland, or a
// headless environment), which the factory turns into an honest capability
// result rather than a crash.
class X11Connection {
 public:
  static std::expected<X11Connection, Error> open();

  ~X11Connection();
  X11Connection(X11Connection&&) noexcept;
  X11Connection& operator=(X11Connection&&) noexcept;
  X11Connection(const X11Connection&) = delete;
  X11Connection& operator=(const X11Connection&) = delete;

  [[nodiscard]] Display* display() const { return display_; }
  [[nodiscard]] Window root() const { return root_; }

 private:
  explicit X11Connection(Display* display);

  Display* display_ = nullptr;
  Window root_ = 0;
};

}  // namespace robot::x11
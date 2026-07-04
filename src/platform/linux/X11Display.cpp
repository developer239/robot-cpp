#include "X11Display.h"

#include <utility>

namespace robot::x11 {

std::expected<X11Connection, Error> X11Connection::open() {
  Display* display = XOpenDisplay(nullptr);
  if (display == nullptr) {
    return std::unexpected(Error::backendUnavailable(
        "cannot open an X display (no X server, or a Wayland session without "
        "Xwayland); set DISPLAY or use the uinput backend"
    ));
  }
  return X11Connection{display};
}

X11Connection::X11Connection(Display* display)
    : display_(display), root_(DefaultRootWindow(display)) {}

X11Connection::~X11Connection() {
  if (display_ != nullptr) XCloseDisplay(display_);
}

X11Connection::X11Connection(X11Connection&& other) noexcept
    : display_(std::exchange(other.display_, nullptr)),
      root_(std::exchange(other.root_, 0)) {}

X11Connection& X11Connection::operator=(X11Connection&& other) noexcept {
  if (this != &other) {
    if (display_ != nullptr) XCloseDisplay(display_);
    display_ = std::exchange(other.display_, nullptr);
    root_ = std::exchange(other.root_, 0);
  }
  return *this;
}

}  // namespace robot::x11
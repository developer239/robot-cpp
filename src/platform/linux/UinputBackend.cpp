#include "UinputBackend.h"

#include <fcntl.h>
#include <linux/uinput.h>
#include <unistd.h>

#include <cmath>
#include <cstring>

#include "LinuxEvdevKeymap.h"  // Key -> evdev KEY_* codes (declared below).

namespace robot::linux_uinput {
namespace {

std::expected<void, Error> writeEvent(
    const int fd, const std::uint16_t type, const std::uint16_t code,
    const std::int32_t value
) {
  input_event ev{};
  ev.type = type;
  ev.code = code;
  ev.value = value;
  if (write(fd, &ev, sizeof(ev)) != static_cast<ssize_t>(sizeof(ev))) {
    return std::unexpected(Error::platformError("write to /dev/uinput"));
  }
  return {};
}

std::uint16_t evdevButton(const MouseButton b) {
  switch (b) {
    case MouseButton::Left: return BTN_LEFT;
    case MouseButton::Right: return BTN_RIGHT;
    case MouseButton::Middle: return BTN_MIDDLE;
    case MouseButton::X1: return BTN_SIDE;
    case MouseButton::X2: return BTN_EXTRA;
  }
  return BTN_LEFT;
}

}  // namespace

std::expected<std::unique_ptr<UinputBackend>, Error> UinputBackend::create() {
  const int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    return std::unexpected(Error::permissionDenied(
        "cannot open /dev/uinput; run as root or add a udev rule granting the "
        "input group write access"
    ));
  }

  // Advertise the event types this virtual device emits: keys, relative motion,
  // and wheel. Enable the whole key range because this backend maps the public
  // key vocabulary to evdev codes at call time.
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_EVBIT, EV_REL);
  ioctl(fd, UI_SET_EVBIT, EV_SYN);
  ioctl(fd, UI_SET_RELBIT, REL_X);
  ioctl(fd, UI_SET_RELBIT, REL_Y);
  ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
  ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
  for (int code = 0; code < KEY_MAX; ++code) {
    ioctl(fd, UI_SET_KEYBIT, code);
  }
  ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
  ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
  ioctl(fd, UI_SET_KEYBIT, BTN_SIDE);
  ioctl(fd, UI_SET_KEYBIT, BTN_EXTRA);

  uinput_setup setup{};
  setup.id.bustype = BUS_USB;
  setup.id.vendor = 0x1;
  setup.id.product = 0x1;
  std::strncpy(setup.name, "robot-cpp virtual input", UINPUT_MAX_NAME_SIZE - 1);

  if (ioctl(fd, UI_DEV_SETUP, &setup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
    close(fd);
    return std::unexpected(Error::platformError("UI_DEV_SETUP / UI_DEV_CREATE"));
  }

  return std::unique_ptr<UinputBackend>(new UinputBackend(fd));
}

UinputBackend::~UinputBackend() {
  if (fd_ >= 0) {
    ioctl(fd_, UI_DEV_DESTROY);
    close(fd_);
  }
}

std::expected<void, Error> UinputBackend::emit(
    const std::uint16_t type, const std::uint16_t code, const std::int32_t value
) {
  if (auto r = writeEvent(fd_, type, code, value); !r) return r;
  return writeEvent(fd_, EV_SYN, SYN_REPORT, 0);
}

std::expected<void, Error> UinputBackend::keyDown(const Key key) {
  const auto code = linux_evdev::keyToEvdev(key);
  if (!code) return std::unexpected(Error::unmappableInput(toString(key)));
  return emit(EV_KEY, *code, 1);
}

std::expected<void, Error> UinputBackend::keyUp(const Key key) {
  const auto code = linux_evdev::keyToEvdev(key);
  if (!code) return std::unexpected(Error::unmappableInput(toString(key)));
  return emit(EV_KEY, *code, 0);
}

std::expected<void, Error> UinputBackend::typeUnicode(char32_t /*codepoint*/) {
  // uinput carries no layout, so mapping a Unicode scalar to a keycode sequence
  // is not possible here. Reported rather than approximated.
  return std::unexpected(Error::unsupported(
      "Unicode text injection is unavailable through uinput; use physical keys"
  ));
}

std::expected<void, Error> UinputBackend::warpCursor(LogicalPoint /*point*/) {
  return std::unexpected(Error::unsupported(
      "absolute cursor positioning is unavailable through uinput (relative "
      "motion only)"
  ));
}

std::expected<LogicalPoint, Error> UinputBackend::cursorPosition() {
  return std::unexpected(Error::unsupported(
      "reading the global pointer position is unavailable through uinput"
  ));
}

std::expected<void, Error> UinputBackend::button(
    const MouseButton button, const ButtonAction action, int /*clickCount*/
) {
  return emit(EV_KEY, evdevButton(button), action == ButtonAction::Down ? 1 : 0);
}

std::expected<void, Error> UinputBackend::scroll(const ScrollDelta delta) {
  if (delta.unit == ScrollUnit::Pixel) {
    return std::unexpected(Error::unsupported(
        "pixel-precise scrolling is unavailable through uinput; use line units"
    ));
  }

  if (delta.vertical != 0.0) {
    if (auto r = emit(EV_REL, REL_WHEEL,
                      static_cast<std::int32_t>(std::lround(delta.vertical)));
        !r) {
      return r;
    }
  }
  if (delta.horizontal != 0.0) {
    if (auto r = emit(EV_REL, REL_HWHEEL,
                      static_cast<std::int32_t>(std::lround(delta.horizontal)));
        !r) {
      return r;
    }
  }
  return {};
}

}  // namespace robot::linux_uinput

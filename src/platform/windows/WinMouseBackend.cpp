#include "WinMouseBackend.h"

#include <cmath>

namespace robot::win {
namespace {

std::expected<void, Error> send(const INPUT& input) {
  INPUT copy = input;
  if (SendInput(1, &copy, sizeof(INPUT)) != 1) {
    return std::unexpected(
        Error::platformError("SendInput", static_cast<long>(GetLastError()))
    );
  }
  return {};
}

}  // namespace

std::expected<void, Error> WinMouseBackend::warpCursor(const LogicalPoint point) {
  // Normalize to 0..65535 across the virtual desktop. Absolute SendInput
  // coordinates are relative to the virtual-screen origin, so a monitor left of
  // or above the primary (negative origin) maps correctly.
  const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
  const int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
  const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  if (vw <= 1 || vh <= 1) {
    return std::unexpected(Error::platformError("invalid virtual screen size"));
  }

  const double nx = (point.x - vx) * 65535.0 / (vw - 1);
  const double ny = (point.y - vy) * 65535.0 / (vh - 1);

  INPUT input{};
  input.type = INPUT_MOUSE;
  input.mi.dx = static_cast<LONG>(std::lround(nx));
  input.mi.dy = static_cast<LONG>(std::lround(ny));
  input.mi.dwFlags =
      MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
  return send(input);
}

std::expected<LogicalPoint, Error> WinMouseBackend::cursorPosition() {
  POINT p{};
  if (GetCursorPos(&p) == 0) {
    return std::unexpected(
        Error::platformError("GetCursorPos", static_cast<long>(GetLastError()))
    );
  }
  return LogicalPoint{static_cast<double>(p.x), static_cast<double>(p.y)};
}

std::expected<void, Error> WinMouseBackend::button(
    const MouseButton button, const ButtonAction action, int /*clickCount*/
) {
  // Windows derives double-clicks from timing and position between successive
  // clicks rather than an explicit count, so clickCount is not forwarded here;
  // the facade's two back-to-back clicks land within the system double-click
  // interval and are recognized natively.
  const bool down = action == ButtonAction::Down;

  INPUT input{};
  input.type = INPUT_MOUSE;

  switch (button) {
    case MouseButton::Left:
      input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
      break;
    case MouseButton::Right:
      input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
      break;
    case MouseButton::Middle:
      input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
      break;
    case MouseButton::X1:
      input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
      input.mi.mouseData = XBUTTON1;
      break;
    case MouseButton::X2:
      input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
      input.mi.mouseData = XBUTTON2;
      break;
  }

  if (auto r = send(input); !r) return r;
  if (down) {
    pressedButton_ = button;
  } else {
    pressedButton_.reset();
  }
  return {};
}

std::expected<void, Error> WinMouseBackend::scroll(const ScrollDelta delta) {
  // WHEEL_DELTA (120) is one notch. Line deltas scale by WHEEL_DELTA; pixel
  // deltas are passed through as raw wheel units, which is the finest resolution
  // the SendInput wheel API exposes (true per-pixel high-resolution scrolling is
  // only available to the Windows Precision Touchpad driver stack, not to
  // synthetic SendInput, so pixel mode here is best-effort granular rather than
  // sub-notch). Vertical > 0 scrolls up, matching the library convention.
  const auto scale = [&](const double v) -> LONG {
    if (delta.unit == ScrollUnit::Line) {
      return static_cast<LONG>(std::lround(v * WHEEL_DELTA));
    }
    return static_cast<LONG>(std::lround(v));
  };

  if (delta.vertical != 0.0) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(scale(delta.vertical));
    if (auto r = send(input); !r) return r;
  }
  if (delta.horizontal != 0.0) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
    input.mi.mouseData = static_cast<DWORD>(scale(delta.horizontal));
    if (auto r = send(input); !r) return r;
  }
  return {};
}

}  // namespace robot::win
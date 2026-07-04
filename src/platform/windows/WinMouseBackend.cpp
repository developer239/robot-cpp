#include "WinMouseBackend.h"

#include <shellscalingapi.h>

#include <cmath>
#include <vector>

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

double monitorScale(HMONITOR monitor) {
  using GetDpiFn = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
  static GetDpiFn getDpi = [] {
    if (HMODULE shcore = LoadLibraryW(L"Shcore.dll"); shcore != nullptr) {
      return reinterpret_cast<GetDpiFn>(
          reinterpret_cast<void*>(GetProcAddress(shcore, "GetDpiForMonitor"))
      );
    }
    return static_cast<GetDpiFn>(nullptr);
  }();

  if (getDpi == nullptr) return 1.0;
  UINT dpiX = 96;
  UINT dpiY = 96;
  if (getDpi(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY) != S_OK) return 1.0;
  return static_cast<double>(dpiX) / 96.0;
}

struct MonitorMapping {
  RECT physical{};
  LogicalRect logical;
  double scale = 1.0;
};

BOOL CALLBACK collectMonitor(
    HMONITOR monitor, HDC /*dc*/, LPRECT /*rect*/, LPARAM userData
) {
  auto* out = reinterpret_cast<std::vector<MonitorMapping>*>(userData);

  MONITORINFO info{};
  info.cbSize = sizeof(info);
  if (GetMonitorInfoW(monitor, &info) == 0) return TRUE;

  const RECT& r = info.rcMonitor;
  const double scale = monitorScale(monitor);
  out->push_back(MonitorMapping{
      .physical = r,
      .logical = LogicalRect{
          {static_cast<double>(r.left) / scale,
           static_cast<double>(r.top) / scale},
          {static_cast<double>(r.right - r.left) / scale,
           static_cast<double>(r.bottom - r.top) / scale}},
      .scale = scale,
  });
  return TRUE;
}

std::expected<std::vector<MonitorMapping>, Error> monitorMappings() {
  std::vector<MonitorMapping> monitors;
  EnumDisplayMonitors(
      nullptr, nullptr, &collectMonitor, reinterpret_cast<LPARAM>(&monitors)
  );
  if (monitors.empty()) {
    return std::unexpected(Error::platformError("EnumDisplayMonitors found none"));
  }
  return monitors;
}

bool contains(const RECT& rect, const POINT point) {
  return point.x >= rect.left && point.x < rect.right && point.y >= rect.top &&
         point.y < rect.bottom;
}

std::expected<POINT, Error> toPhysicalPoint(const LogicalPoint point) {
  auto monitors = monitorMappings();
  if (!monitors) return std::unexpected(monitors.error());

  for (const MonitorMapping& monitor : *monitors) {
    if (!monitor.logical.contains(point)) continue;
    return POINT{static_cast<LONG>(std::lround(point.x * monitor.scale)),
                 static_cast<LONG>(std::lround(point.y * monitor.scale))};
  }
  return std::unexpected(Error::invalidArgument(
      "logical cursor point is outside all enumerated monitors"
  ));
}

std::expected<LogicalPoint, Error> toLogicalPoint(const POINT point) {
  auto monitors = monitorMappings();
  if (!monitors) return std::unexpected(monitors.error());

  for (const MonitorMapping& monitor : *monitors) {
    if (!contains(monitor.physical, point)) continue;
    return LogicalPoint{static_cast<double>(point.x) / monitor.scale,
                        static_cast<double>(point.y) / monitor.scale};
  }
  return std::unexpected(Error::platformError(
      "physical cursor position is outside all enumerated monitors"
  ));
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

  auto physical = toPhysicalPoint(point);
  if (!physical) return std::unexpected(physical.error());

  const double nx = (physical->x - vx) * 65535.0 / (vw - 1);
  const double ny = (physical->y - vy) * 65535.0 / (vh - 1);

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
  return toLogicalPoint(p);
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
  if (delta.unit == ScrollUnit::Pixel) {
    return std::unexpected(Error::unsupported(
        "pixel-precise scrolling is unavailable through SendInput; use line "
        "units"
    ));
  }

  // WHEEL_DELTA (120) is one notch. Vertical > 0 scrolls up, matching the
  // library convention.
  const auto scale = [](const double v) -> LONG {
    return static_cast<LONG>(std::lround(v * WHEEL_DELTA));
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

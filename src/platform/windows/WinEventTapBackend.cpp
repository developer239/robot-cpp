#include "WinEventTapBackend.h"

#include <cmath>

#include "WinKeyMap.h"
#include "robot/Event.h"

namespace robot::win {
namespace {

// The single active tap. A low-level hook callback is a C function pointer with
// no user-data parameter, so exactly one instance can be registered at a time;
// this .cpp-local pointer replaces the header-defined static that caused ODR
// problems. Access is confined to start/stop and the callbacks on the hook
// thread.
WinEventTapBackend* g_active = nullptr;

// Convert a physical-pixel hook position to logical coordinates using the
// primary monitor's DPI. Multi-monitor logical mapping would require locating the
// owning monitor; for recording fidelity the primary scale is used and documented
// as such rather than silently assuming 96 DPI everywhere.
LogicalPoint toLogical(const POINT p) {
  const UINT dpi = GetDpiForSystem();  // Primary/system DPI (Win10 1607+).
  const double scale = dpi > 0 ? static_cast<double>(dpi) / 96.0 : 1.0;
  return LogicalPoint{static_cast<double>(p.x) / scale,
                      static_cast<double>(p.y) / scale};
}

}  // namespace

LRESULT CALLBACK WinEventTapBackend::mouseProc(
    const int code, const WPARAM wParam, const LPARAM lParam
) {
  if (code == HC_ACTION && g_active != nullptr) {
    g_active->onMouse(wParam, *reinterpret_cast<MSLLHOOKSTRUCT*>(lParam));
  }
  return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK WinEventTapBackend::keyboardProc(
    const int code, const WPARAM wParam, const LPARAM lParam
) {
  if (code == HC_ACTION && g_active != nullptr) {
    g_active->onKeyboard(wParam, *reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam));
  }
  return CallNextHookEx(nullptr, code, wParam, lParam);
}

void WinEventTapBackend::onMouse(
    const WPARAM wParam, const MSLLHOOKSTRUCT& data
) {
  const LogicalPoint pos = toLogical(data.pt);
  switch (wParam) {
    case WM_MOUSEMOVE:
      sink_(MouseMoveEvent{pos});
      break;
    case WM_LBUTTONDOWN:
      sink_(MouseButtonEvent{MouseButton::Left, true, pos});
      break;
    case WM_LBUTTONUP:
      sink_(MouseButtonEvent{MouseButton::Left, false, pos});
      break;
    case WM_RBUTTONDOWN:
      sink_(MouseButtonEvent{MouseButton::Right, true, pos});
      break;
    case WM_RBUTTONUP:
      sink_(MouseButtonEvent{MouseButton::Right, false, pos});
      break;
    case WM_MBUTTONDOWN:
      sink_(MouseButtonEvent{MouseButton::Middle, true, pos});
      break;
    case WM_MBUTTONUP:
      sink_(MouseButtonEvent{MouseButton::Middle, false, pos});
      break;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP: {
      const WORD which = GET_XBUTTON_WPARAM(data.mouseData);
      const MouseButton b =
          which == XBUTTON1 ? MouseButton::X1 : MouseButton::X2;
      sink_(MouseButtonEvent{b, wParam == WM_XBUTTONDOWN, pos});
      break;
    }
    case WM_MOUSEWHEEL: {
      const short raw = GET_WHEEL_DELTA_WPARAM(data.mouseData);
      sink_(ScrollEvent{ScrollDelta::lines(
                            static_cast<double>(raw) / WHEEL_DELTA, 0.0),
                        pos});
      break;
    }
    case WM_MOUSEHWHEEL: {
      const short raw = GET_WHEEL_DELTA_WPARAM(data.mouseData);
      sink_(ScrollEvent{ScrollDelta::lines(
                            0.0, static_cast<double>(raw) / WHEEL_DELTA),
                        pos});
      break;
    }
    default:
      break;
  }
}

void WinEventTapBackend::onKeyboard(
    const WPARAM wParam, const KBDLLHOOKSTRUCT& data
) {
  const bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
  const bool up = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
  if (!down && !up) return;

  const bool extended = (data.flags & LLKHF_EXTENDED) != 0;
  const Key key = scanCodeToKey(static_cast<WORD>(data.scanCode), extended);
  if (key != Key::Unknown) {
    sink_(KeyEvent{key, down});
  }
}

std::expected<void, Error> WinEventTapBackend::start(EventSink sink) {
  if (g_active != nullptr) {
    return std::unexpected(
        Error::unsupported("an event tap is already running in this process")
    );
  }
  sink_ = std::move(sink);
  g_active = this;
  threadId_ = GetCurrentThreadId();

  const HINSTANCE mod = GetModuleHandleW(nullptr);
  mouseHook_ = SetWindowsHookExW(WH_MOUSE_LL, &mouseProc, mod, 0);
  keyboardHook_ = SetWindowsHookExW(WH_KEYBOARD_LL, &keyboardProc, mod, 0);
  if (mouseHook_ == nullptr || keyboardHook_ == nullptr) {
    if (mouseHook_ != nullptr) UnhookWindowsHookEx(mouseHook_);
    if (keyboardHook_ != nullptr) UnhookWindowsHookEx(keyboardHook_);
    mouseHook_ = nullptr;
    keyboardHook_ = nullptr;
    g_active = nullptr;
    return std::unexpected(Error::platformError(
        "SetWindowsHookEx", static_cast<long>(GetLastError())
    ));
  }

  running_.store(true);

  // Low-level hooks require a message loop on the installing thread to be
  // dispatched. Block here until stop() posts WM_QUIT to this thread.
  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  running_.store(false);
  UnhookWindowsHookEx(mouseHook_);
  UnhookWindowsHookEx(keyboardHook_);
  mouseHook_ = nullptr;
  keyboardHook_ = nullptr;
  g_active = nullptr;
  return {};
}

void WinEventTapBackend::stop() {
  running_.store(false);
  if (threadId_ != 0) {
    PostThreadMessageW(threadId_, WM_QUIT, 0, 0);
  }
}

}  // namespace robot::win
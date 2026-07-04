#pragma once

#include <string>

namespace robot {

// What the active backend can actually do in the current environment. This is
// how the library reports platform limits up front instead of hiding them behind
// silent fallbacks: a false flag here means the corresponding operation returns
// ErrorCode::Unsupported (or PermissionDenied) when called, never a best-effort
// no-op. Query capabilities() once after creating a Session and branch on it.
//
// Motivating cases the flags make explicit:
//   * Wayland cannot warp the cursor to absolute coordinates or read the global
//     pointer position from an unprivileged client -> canWarpCursor /
//     canReadCursorPosition are false there.
//   * Extra mouse buttons (X1/X2) and pixel-precise scrolling are not injectable
//     on every backend.
//   * macOS requires Accessibility for input injection and Screen Recording for
//     capture; the requires* flags say so before an operation fails.
struct Capabilities {
  std::string backendName;  // e.g. "macOS Quartz", "Windows SendInput", "Linux X11/XTest".

  bool canInjectKeyboard = false;
  bool canInjectMouse = false;
  bool canTypeUnicode = false;  // Layout-independent text injection is available.

  bool canWarpCursor = false;          // Absolute cursor positioning.
  bool canReadCursorPosition = false;  // Query current pointer location.

  bool supportsExtraMouseButtons = false;    // X1 / X2.
  bool supportsHighResolutionScroll = false;  // Pixel-unit scroll deltas.

  bool canCaptureScreen = false;
  bool canEnumerateMonitors = false;
  bool canRecordEvents = false;  // Global input tap / hook.

  bool requiresAccessibilityPermission = false;
  bool requiresScreenRecordingPermission = false;
};

}  // namespace robot
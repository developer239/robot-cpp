#include <cstdlib>
#include <string_view>

#include "LinuxPlatformBackend.h"
#include "UinputBackend.h"
#include "X11Display.h"
#include "robot/backend/BackendFactory.h"

namespace robot::backend {
namespace {

bool waylandSessionActive() {
  const char* wayland = std::getenv("WAYLAND_DISPLAY");
  return wayland != nullptr && wayland[0] != '\0';
}

bool x11DisplayAvailable() {
  const char* display = std::getenv("DISPLAY");
  return display != nullptr && display[0] != '\0';
}

Capabilities x11Capabilities() {
  Capabilities c;
  c.backendName = "Linux X11/XTest";
  c.canInjectKeyboard = true;
  c.canInjectMouse = true;
  c.canTypeUnicode = true;  // Via temporary keysym remap.
  c.canWarpCursor = true;
  c.canReadCursorPosition = true;
  c.supportsExtraMouseButtons = true;
  c.supportsHighResolutionScroll = false;  // Core wheel is discrete buttons.
  c.canCaptureScreen = true;
  c.canEnumerateMonitors = true;
  c.canRecordEvents = true;  // Via XRecord, if the extension is present.
  c.requiresAccessibilityPermission = false;
  c.requiresScreenRecordingPermission = false;
  return c;
}

Capabilities uinputCapabilities() {
  Capabilities c;
  c.backendName = "Linux uinput";
  c.canInjectKeyboard = true;
  c.canInjectMouse = true;
  c.canTypeUnicode = false;       // No layout knowledge at the kernel level.
  c.canWarpCursor = false;        // Relative motion only.
  c.canReadCursorPosition = false;
  c.supportsExtraMouseButtons = true;
  c.supportsHighResolutionScroll = false;
  c.canCaptureScreen = false;
  c.canEnumerateMonitors = false;
  c.canRecordEvents = false;
  c.requiresAccessibilityPermission = false;
  c.requiresScreenRecordingPermission = false;
  return c;
}

std::expected<std::unique_ptr<IPlatformBackend>, Error> makeX11() {
  auto connection = x11::X11Connection::open();
  if (!connection) return std::unexpected(connection.error());
  return std::make_unique<linux_backend::X11PlatformBackend>(
      std::move(*connection), x11Capabilities()
  );
}

std::expected<std::unique_ptr<IPlatformBackend>, Error> makeUinput() {
  auto device = linux_uinput::UinputBackend::create();
  if (!device) return std::unexpected(device.error());
  return std::make_unique<linux_backend::UinputPlatformBackend>(
      std::move(*device), uinputCapabilities()
  );
}

}  // namespace

std::expected<std::unique_ptr<IPlatformBackend>, Error> createPlatformBackend(
    const SessionOptions& options
) {
  // Explicit backend selection always wins and is never second-guessed.
  switch (options.linuxBackend) {
    case LinuxBackend::X11:
      return makeX11();
    case LinuxBackend::Uinput:
      return makeUinput();
    case LinuxBackend::Auto:
      break;
  }

  // Auto: prefer X11 when an X server is reachable. This includes Xwayland, so a
  // Wayland desktop that runs Xwayland still gets full XTest behaviour for X11
  // and Xwayland clients.
  if (x11DisplayAvailable()) {
    if (auto x11 = makeX11(); x11) return x11;
    // DISPLAY was set but the connection failed; fall through to the diagnostics
    // below rather than masking the real environment problem.
  }

  // No usable X server. Be explicit about why, and point to the concrete option
  // instead of silently degrading to a backend that cannot do what the caller
  // expects. This is the honest-limitation requirement made literal.
  if (waylandSessionActive()) {
    return std::unexpected(Error::unsupported(
        "this is a native Wayland session with no reachable X server. Wayland "
        "does not expose a protocol for an unprivileged client to inject input, "
        "warp the cursor, or capture the screen. Options: run under Xwayland "
        "(set DISPLAY), or construct the Session with "
        "LinuxBackend::Uinput for kernel-level input injection (needs "
        "/dev/uinput access; no cursor warp, pointer read, or capture)."
    ));
  }

  return std::unexpected(Error::backendUnavailable(
      "no X server (DISPLAY unset) and no Wayland session detected. Set DISPLAY "
      "for X11, or use LinuxBackend::Uinput for headless kernel-level injection."
  ));
}

}  // namespace robot::backend
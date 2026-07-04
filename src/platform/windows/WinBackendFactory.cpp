#include "WinDpi.h"
#include "WinPlatformBackend.h"
#include "robot/backend/BackendFactory.h"

namespace robot::backend {
namespace {

Capabilities buildCapabilities() {
  Capabilities c;
  c.backendName = "Windows SendInput";

  // Windows requires no special runtime permission for input injection or GDI
  // capture from an interactive desktop session, so these are unconditionally
  // available. (UIPI can still block injection into a higher-integrity target
  // window; that is a per-target boundary, not a session capability, and surfaces
  // as a SendInput failure rather than a capability flag.)
  c.canInjectKeyboard = true;
  c.canInjectMouse = true;
  c.canTypeUnicode = true;
  c.canWarpCursor = true;
  c.canReadCursorPosition = true;
  c.supportsExtraMouseButtons = true;
  c.supportsHighResolutionScroll = true;  // Granular wheel via SendInput.
  c.canCaptureScreen = true;
  c.canEnumerateMonitors = true;
  c.canRecordEvents = true;

  c.requiresAccessibilityPermission = false;
  c.requiresScreenRecordingPermission = false;
  return c;
}

}  // namespace

std::expected<std::unique_ptr<IPlatformBackend>, Error> createPlatformBackend(
    const SessionOptions& /*options*/
) {
  // Declare DPI awareness before any coordinate or capture work so every metric
  // this backend reads and writes is in true physical pixels. This is the single
  // correct place for it: once, at backend creation, before a Session exists.
  win::ensurePerMonitorDpiAwareness();

  // No permission preflight is required on Windows; requireInputPermission /
  // requireCapturePermission in SessionOptions are satisfied by the interactive
  // session itself.
  return std::make_unique<win::WinPlatformBackend>(buildCapabilities());
}

}  // namespace robot::backend
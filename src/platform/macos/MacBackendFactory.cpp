#include <ApplicationServices/ApplicationServices.h>

#include "MacPlatformBackend.h"
#include "robot/backend/BackendFactory.h"

namespace robot::backend {
namespace {

// Whether Accessibility is currently granted. This gates all event injection and
// the event tap; without it, CGEventPost is silently dropped by the OS, so the
// only honest place to surface the problem is the capability report.
bool accessibilityGranted() { return AXIsProcessTrusted() != 0; }

// Whether Screen Recording is currently granted (macOS 10.15+).
bool screenRecordingGranted() {
  return CGPreflightScreenCaptureAccess() != 0;
}

Capabilities buildCapabilities(const bool ax, const bool sr) {
  Capabilities c;
  c.backendName = "macOS Quartz";

  // Injection and recording depend on Accessibility being granted right now;
  // reflecting that here means capabilities() tells the truth about this
  // environment rather than advertising abilities that will silently no-op.
  c.canInjectKeyboard = ax;
  c.canInjectMouse = ax;
  c.canTypeUnicode = ax;
  c.canWarpCursor = ax;
  c.canRecordEvents = ax;

  c.canReadCursorPosition = true;  // No special permission needed.
  c.supportsExtraMouseButtons = true;
  c.supportsHighResolutionScroll = true;
  c.canEnumerateMonitors = true;

  c.canCaptureScreen = sr;

  c.requiresAccessibilityPermission = true;
  c.requiresScreenRecordingPermission = true;
  return c;
}

}  // namespace

std::expected<std::unique_ptr<IPlatformBackend>, Error> createPlatformBackend(
    const SessionOptions& options
) {
  const bool ax = accessibilityGranted();
  const bool sr = screenRecordingGranted();

  // Fail up front when the caller demanded a permission that is not granted,
  // rather than surfacing it at first injection or capture.
  if (options.requireInputPermission && !ax) {
    return std::unexpected(Error::permissionDenied(
        "Accessibility is required for input injection; grant it in System "
        "Settings > Privacy & Security > Accessibility"
    ));
  }
  if (options.requireCapturePermission && !sr) {
    return std::unexpected(Error::permissionDenied(
        "Screen Recording is required for capture; grant it in System "
        "Settings > Privacy & Security > Screen Recording"
    ));
  }

  return std::make_unique<mac::MacPlatformBackend>(buildCapabilities(ax, sr));
}

}  // namespace robot::backend
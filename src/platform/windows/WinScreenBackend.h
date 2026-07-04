#pragma once

#include <Windows.h>

#include <vector>

#include "robot/backend/IScreenBackend.h"

namespace robot::win {

// GDI display enumeration and capture. Each monitor is reported with its own DPI
// (GetDpiForMonitor), so scaleFactor is per-display rather than a single global
// value - correct for mixed-DPI setups where a 4K panel at 150% sits beside a
// 1080p panel at 100%. Bounds come from the monitor rectangle, which under
// Per-Monitor-V2 awareness is in physical pixels and can be negative for
// monitors placed left of or above the primary. Capture uses BitBlt + GetDIBits
// and converts the returned bottom-up BGR into canonical top-down straight RGBA.
class WinScreenBackend final : public backend::IScreenBackend {
 public:
  std::expected<std::vector<Monitor>, Error> enumerateMonitors() override;
  std::expected<Image, Error> captureRegion(PhysicalRect region) override;

 private:
  static BOOL CALLBACK monitorEnumProc(
      HMONITOR monitor, HDC dc, LPRECT rect, LPARAM userData
  );
};

}  // namespace robot::win
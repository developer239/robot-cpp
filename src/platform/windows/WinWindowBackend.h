#pragma once

#include "robot/backend/IWindowBackend.h"

namespace robot::win {

// Window enumeration through EnumWindows, and capture through Windows Graphics
// Capture. Under Per-Monitor-V2 awareness window rectangles are physical pixels,
// so frames divide them by the window's own DPI scale to report logical points.
class WinWindowBackend final : public backend::IWindowBackend {
 public:
  std::expected<std::vector<Window>, Error> enumerateWindows() override;

  std::expected<LogicalRect, Error> windowFrame(std::uint32_t windowId) override;

  std::expected<std::optional<std::uint32_t>, Error> topmostWindowId() override;

  std::expected<void, Error> activateApplicationForWindow(
      std::uint32_t windowId
  ) override;

  std::expected<std::unique_ptr<backend::IWindowStreamBackend>, Error>
  startStream(
      std::uint32_t windowId, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError
  ) override;

  std::expected<std::unique_ptr<backend::IWindowStreamBackend>, Error>
  startMonitorStream(
      std::uint32_t monitorId, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError
  ) override;
};

}  // namespace robot::win

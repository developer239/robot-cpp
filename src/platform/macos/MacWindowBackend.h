#pragma once

#include "robot/backend/IWindowBackend.h"

namespace robot::mac {

// ScreenCaptureKit-backed window enumeration and streaming, CoreGraphics
// window/focus queries, AppKit activation. Stateless: every stream returned by
// startStream is a self-contained handle owning its own delivery queue and
// lifetime channel, so the backend itself carries no per-stream state. This
// header stays free of ObjC types so C++ translation units can include it; the
// implementation lives in the .mm.
class MacWindowBackend final : public backend::IWindowBackend {
 public:
  [[nodiscard]] std::expected<std::vector<Window>, Error> enumerateWindows()
      override;

  [[nodiscard]] std::expected<LogicalRect, Error> windowFrame(
      std::uint32_t windowId
  ) override;

  [[nodiscard]] std::expected<std::optional<std::uint32_t>, Error>
  topmostWindowId() override;

  [[nodiscard]] std::expected<void, Error> activateApplicationForWindow(
      std::uint32_t windowId
  ) override;

  [[nodiscard]] std::expected<
      std::unique_ptr<backend::IWindowStreamBackend>, Error>
  startStream(
      std::uint32_t windowId, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError
  ) override;
};

}  // namespace robot::mac

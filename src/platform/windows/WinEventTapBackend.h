#pragma once

#include <Windows.h>

#include <atomic>

#include "robot/backend/IEventTapBackend.h"

namespace robot::win {

// Low-level global hooks (WH_MOUSE_LL, WH_KEYBOARD_LL) translating native events
// into normalized InputEvents. Key events are translated by scan code into
// physical Key values (layout-independent recording), and mouse positions are
// reported in logical coordinates (physical pixels divided by the primary DPI
// scale) to match the injection side.
//
// The previous EventHook stored its instance pointer in a static defined in a
// header, an ODR violation that would multiply-define across translation units;
// here the active instance is registered through a single .cpp-local pointer, and
// the hooks are torn down and the pointer cleared on stop.
class WinEventTapBackend final : public backend::IEventTapBackend {
 public:
  WinEventTapBackend() = default;
  ~WinEventTapBackend() override = default;

  WinEventTapBackend(const WinEventTapBackend&) = delete;
  WinEventTapBackend& operator=(const WinEventTapBackend&) = delete;

  std::expected<void, Error> start(EventSink sink) override;
  void stop() override;
  [[nodiscard]] bool isRunning() const override { return running_.load(); }

 private:
  static LRESULT CALLBACK mouseProc(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam);

  void onMouse(WPARAM wParam, const MSLLHOOKSTRUCT& data);
  void onKeyboard(WPARAM wParam, const KBDLLHOOKSTRUCT& data);

  EventSink sink_;
  HHOOK mouseHook_ = nullptr;
  HHOOK keyboardHook_ = nullptr;
  DWORD threadId_ = 0;
  std::atomic<bool> running_{false};
};

}  // namespace robot::win
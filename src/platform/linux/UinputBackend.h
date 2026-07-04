#pragma once

#include <memory>

#include "robot/backend/IKeyboardBackend.h"
#include "robot/backend/IMouseBackend.h"

namespace robot::linux_uinput {

// Kernel-level input injection via /dev/uinput. This creates a virtual input
// device and emits evdev events, which the compositor consumes like any hardware
// device - so, unlike XTest, it works under a native Wayland session. The
// trade-offs are inherent to the kernel interface and are reported honestly in
// Capabilities rather than papered over:
//
//   * It requires write access to /dev/uinput (root, or an appropriate udev rule
//     / input group). Open failure is a hard PermissionDenied.
//   * It injects RELATIVE pointer motion. There is no absolute-cursor-warp and no
//     way to read the global pointer position through uinput, so warpCursor and
//     cursorPosition return Unsupported. Absolute positioning would require the
//     compositor's cooperation, which uinput does not have.
//   * Keys are emitted as evdev keycodes; text is produced by mapping Unicode to
//     a key sequence is NOT attempted here (that needs layout knowledge uinput
//     does not carry), so typeUnicode reports Unsupported and callers use
//     physical keys.
//
// One class implements both keyboard and mouse because they share the single
// virtual device fd.
class UinputBackend final : public backend::IKeyboardBackend,
                            public backend::IMouseBackend {
 public:
  static std::expected<std::unique_ptr<UinputBackend>, Error> create();
  ~UinputBackend() override;

  UinputBackend(const UinputBackend&) = delete;
  UinputBackend& operator=(const UinputBackend&) = delete;

  // Keyboard.
  std::expected<void, Error> keyDown(Key key) override;
  std::expected<void, Error> keyUp(Key key) override;
  std::expected<void, Error> typeUnicode(char32_t codepoint) override;

  // Mouse.
  std::expected<void, Error> warpCursor(LogicalPoint point) override;
  std::expected<LogicalPoint, Error> cursorPosition() override;
  std::expected<void, Error> button(
      MouseButton button, ButtonAction action, int clickCount
  ) override;
  std::expected<void, Error> scroll(ScrollDelta delta) override;

 private:
  explicit UinputBackend(int fd) : fd_(fd) {}
  std::expected<void, Error> emit(
      std::uint16_t type, std::uint16_t code, std::int32_t value
  );

  int fd_ = -1;
};

}  // namespace robot::linux_uinput

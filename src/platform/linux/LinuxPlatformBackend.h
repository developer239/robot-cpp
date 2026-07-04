#pragma once

#include <memory>

#include "UinputBackend.h"
#include "X11Display.h"
#include "X11EventTapBackend.h"
#include "X11KeyboardBackend.h"
#include "X11MouseBackend.h"
#include "X11ScreenBackend.h"
#include "robot/backend/IPlatformBackend.h"

namespace robot::linux_backend {

// The X11 platform assembly: owns the shared connection and the four X11 sub-
// backends. Constructed only after the factory has verified an X server is
// reachable, so its sub-backends can assume a valid connection.
class X11PlatformBackend final : public backend::IPlatformBackend {
 public:
  X11PlatformBackend(x11::X11Connection connection, Capabilities capabilities);

  backend::IKeyboardBackend& keyboard() override { return *keyboard_; }
  backend::IMouseBackend& mouse() override { return *mouse_; }
  backend::IScreenBackend& screen() override { return *screen_; }
  backend::IEventTapBackend* eventTap() override { return eventTap_.get(); }
  const Capabilities& capabilities() const override { return capabilities_; }

 private:
  x11::X11Connection connection_;
  Capabilities capabilities_;
  std::unique_ptr<x11::X11KeyboardBackend> keyboard_;
  std::unique_ptr<x11::X11MouseBackend> mouse_;
  std::unique_ptr<x11::X11ScreenBackend> screen_;
  std::unique_ptr<x11::X11EventTapBackend> eventTap_;
};

// The uinput platform assembly: kernel-level injection that works under Wayland
// but has no capture, no absolute cursor, and no monitor enumeration. Those
// missing abilities are reported in Capabilities and return Unsupported when
// called; screen()/eventTap() are backed by a null-capability screen and no tap.
class UinputPlatformBackend final : public backend::IPlatformBackend {
 public:
  UinputPlatformBackend(
      std::unique_ptr<linux_uinput::UinputBackend> device,
      Capabilities capabilities
  );

  backend::IKeyboardBackend& keyboard() override { return *device_; }
  backend::IMouseBackend& mouse() override { return *device_; }
  backend::IScreenBackend& screen() override { return *screen_; }
  backend::IEventTapBackend* eventTap() override { return nullptr; }
  const Capabilities& capabilities() const override { return capabilities_; }

 private:
  std::unique_ptr<linux_uinput::UinputBackend> device_;
  std::unique_ptr<backend::IScreenBackend> screen_;
  Capabilities capabilities_;
};

}  // namespace robot::linux_backend
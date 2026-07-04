#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <vector>

#include "robot/Capabilities.h"
#include "robot/Error.h"
#include "robot/EventTap.h"
#include "robot/Keyboard.h"
#include "robot/Monitor.h"
#include "robot/Mouse.h"
#include "robot/Screen.h"

namespace robot {
namespace backend {
class IPlatformBackend;
}

// Which Linux backend to select. Ignored on macOS and Windows.
enum class LinuxBackend : std::uint8_t {
  // Pick the best available at runtime: X11/XTest under an X session, otherwise
  // report the Wayland limitation honestly.
  Auto,
  // Force X11/XTest (also works for XWayland apps under an Xwayland-backed
  // session).
  X11,
  // Force the uinput virtual-device backend (needs /dev/uinput write access).
  // This injects at the kernel level and so also works under a Wayland
  // compositor, but cannot warp the absolute cursor or read pointer position.
  Uinput,
};

struct SessionOptions {
  // When true, create() fails with PermissionDenied if the platform's input
  // permission (macOS Accessibility) is not already granted, instead of
  // succeeding and surfacing the error at first injection. Capture permission is
  // handled the same way when captureRequiresPermission applies.
  bool requireInputPermission = false;
  bool requireCapturePermission = false;
  LinuxBackend linuxBackend = LinuxBackend::Auto;
};

// The single entry point and the sole owner of platform state. There is no
// global or static input state anywhere in the library: everything hangs off a
// Session, so lifetimes are explicit and the whole stack is mockable by
// substituting a backend.
//
// create() performs all fallible initialization (backend selection, permission
// checks, display server probing) and returns a fully-formed Session or a
// specific Error - it never hands back a half-initialized object. The subsystem
// facades it exposes borrow the Session-owned backend, so the Session must
// outlive any reference taken from it. Non-copyable and non-movable to keep those
// borrowed references stable.
class Session {
 public:
  [[nodiscard]] static std::expected<std::unique_ptr<Session>, Error> create(
      const SessionOptions& options = {}
  );

  ~Session();
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
  Session(Session&&) = delete;
  Session& operator=(Session&&) = delete;

  [[nodiscard]] Keyboard& keyboard() { return keyboard_; }
  [[nodiscard]] Mouse& mouse() { return mouse_; }
  [[nodiscard]] Screen& screen() { return screen_; }

  // Always present; start() reports Unsupported if this platform build has no tap
  // implementation (see EventTap).
  [[nodiscard]] EventTap& eventTap() { return eventTap_; }

  [[nodiscard]] const Capabilities& capabilities() const {
    return capabilities_;
  }

  [[nodiscard]] std::expected<std::vector<Monitor>, Error> monitors() {
    return screen_.monitors();
  }

 private:
  struct PrivateTag {};

 public:
  // Public for make_unique but uncallable externally: PrivateTag is private, so
  // only create() can construct a Session.
  Session(PrivateTag, std::unique_ptr<backend::IPlatformBackend> backend);

 private:
  std::unique_ptr<backend::IPlatformBackend> backend_;
  Capabilities capabilities_;
  Keyboard keyboard_;
  Mouse mouse_;
  Screen screen_;
  EventTap eventTap_;
};

}  // namespace robot
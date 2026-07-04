#include "X11KeyboardBackend.h"

#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

#include "X11KeyMap.h"

namespace robot::x11 {
namespace {

// Map a Unicode scalar to the X keysym convention: Latin-1 is its own codepoint,
// everything else is 0x01000000 | codepoint. This is what a temporary remap binds
// so any character (CJK, emoji, accented) can be injected.
KeySym unicodeToKeysym(const char32_t cp) {
  if (cp < 0x100) return static_cast<KeySym>(cp);
  return static_cast<KeySym>(0x01000000u | cp);
}

}  // namespace

std::expected<KeyCode, Error> X11KeyboardBackend::resolveKeycode(const Key key) {
  const KeySym ks = keyToKeysym(key);
  if (ks == NoSymbol) {
    return std::unexpected(Error::unmappableInput(toString(key)));
  }
  const KeyCode code = XKeysymToKeycode(connection_->display(), ks);
  if (code == 0) {
    return std::unexpected(Error::unmappableInput(toString(key)));
  }
  return code;
}

std::expected<void, Error> X11KeyboardBackend::keyDown(const Key key) {
  auto code = resolveKeycode(key);
  if (!code) return std::unexpected(code.error());
  XTestFakeKeyEvent(connection_->display(), *code, True, CurrentTime);
  XFlush(connection_->display());
  return {};
}

std::expected<void, Error> X11KeyboardBackend::keyUp(const Key key) {
  auto code = resolveKeycode(key);
  if (!code) return std::unexpected(code.error());
  XTestFakeKeyEvent(connection_->display(), *code, False, CurrentTime);
  XFlush(connection_->display());
  return {};
}

std::expected<void, Error> X11KeyboardBackend::typeUnicode(
    const char32_t codepoint
) {
  Display* dpy = connection_->display();

  // Borrow a spare keycode, bind the target keysym to it, tap it, then restore.
  // Keycode 1 (unused on standard layouts) is a conventional scratch slot; the
  // original mapping is read first and written back so no permanent change leaks.
  const KeyCode scratch = 1;
  const KeySym target = unicodeToKeysym(codepoint);

  int perCode = 0;
  KeySym* original =
      XGetKeyboardMapping(dpy, scratch, 1, &perCode);
  if (original == nullptr || perCode < 1) {
    if (original != nullptr) XFree(original);
    return std::unexpected(
        Error::platformError("XGetKeyboardMapping for Unicode injection")
    );
  }

  KeySym remap = target;
  XChangeKeyboardMapping(dpy, scratch, 1, &remap, 1);
  XSync(dpy, False);

  XTestFakeKeyEvent(dpy, scratch, True, CurrentTime);
  XTestFakeKeyEvent(dpy, scratch, False, CurrentTime);
  XSync(dpy, False);

  XChangeKeyboardMapping(dpy, scratch, perCode, original, 1);
  XSync(dpy, False);
  XFree(original);
  return {};
}

}  // namespace robot::x11
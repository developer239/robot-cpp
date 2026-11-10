#include "MacKeyboardBackend.h"

#include <array>

#include "MacKeyLayout.h"
#include "MacKeyMap.h"

namespace robot::mac {
namespace {

// Encode one Unicode scalar as UTF-16 (one unit, or a surrogate pair above the
// BMP). Callers validated the scalar, so no error path is needed here.
int toUtf16(const char32_t cp, std::array<UniChar, 2>& out) {
  if (cp <= 0xFFFF) {
    out[0] = static_cast<UniChar>(cp);
    return 1;
  }
  const char32_t v = cp - 0x10000;
  out[0] = static_cast<UniChar>(0xD800 + (v >> 10));
  out[1] = static_cast<UniChar>(0xDC00 + (v & 0x3FF));
  return 2;
}

}  // namespace

MacKeyboardBackend::MacKeyboardBackend() {
  // Private source state keeps synthetic modifiers from entangling with real
  // hardware state; flags are set explicitly per event regardless.
  source_ = CGEventSourceCreate(kCGEventSourceStatePrivate);
  if (source_ != nullptr) {
    // Do not suppress the caller's own subsequent local events after injection.
    CGEventSourceSetLocalEventsSuppressionInterval(source_, 0.0);
  }
}

MacKeyboardBackend::~MacKeyboardBackend() {
  if (source_ != nullptr) CFRelease(source_);
}

CGEventFlags MacKeyboardBackend::currentFlags() const {
  CGEventFlags flags = 0;
  for (const Key k : heldModifiers_) flags |= macModifierFlag(k);
  return flags;
}

std::expected<void, Error> MacKeyboardBackend::postKey(
    const Key key, const bool down
) {
  const auto keycode = keyToMacKeycode(key);
  if (!keycode) {
    return std::unexpected(Error::unmappableInput(toString(key)));
  }

  // Update held-modifier state before computing flags so a modifier's own event
  // carries (down) or clears (up) its flag consistently.
  if (isModifierKey(key)) {
    if (down) {
      heldModifiers_.insert(key);
    } else {
      heldModifiers_.erase(key);
    }
  }

  CGEventRef event = CGEventCreateKeyboardEvent(source_, *keycode, down);
  if (event == nullptr) {
    return std::unexpected(Error::platformError("CGEventCreateKeyboardEvent"));
  }
  CGEventSetFlags(event, currentFlags());
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  return {};
}

std::expected<void, Error> MacKeyboardBackend::keyDown(const Key key) {
  return postKey(key, true);
}

std::expected<void, Error> MacKeyboardBackend::keyUp(const Key key) {
  return postKey(key, false);
}

std::expected<void, Error> MacKeyboardBackend::typeUnicode(
    const char32_t codepoint
) {
  std::array<UniChar, 2> utf16{};
  const int len = toUtf16(codepoint, utf16);

  // The character is delivered by value through the attached Unicode string, on
  // the key of the current layout that types it, with that key's Shift or
  // Option: Chromium derives a keyup's key from its keycode, so a keyup on any
  // other key would name a different key than the keydown did. A character no
  // key types goes out on the text-only key. Held modifiers are intentionally
  // not applied to text (typing 'A' should not also send Command).
  const auto stroke = keyStrokeForCharacter(codepoint);
  const CGKeyCode keycode = stroke ? stroke->keycode : kTextOnlyKeycode;
  const CGEventFlags flags = stroke ? stroke->flags : 0;
  CGEventRef down = CGEventCreateKeyboardEvent(source_, keycode, true);
  CGEventRef up = CGEventCreateKeyboardEvent(source_, keycode, false);
  if (down == nullptr || up == nullptr) {
    if (down != nullptr) CFRelease(down);
    if (up != nullptr) CFRelease(up);
    return std::unexpected(Error::platformError("CGEventCreateKeyboardEvent"));
  }

  CGEventKeyboardSetUnicodeString(
      down, static_cast<UniCharCount>(len), utf16.data()
  );
  CGEventKeyboardSetUnicodeString(
      up, static_cast<UniCharCount>(len), utf16.data()
  );
  // A new event's default flags come from the source's private state, which
  // can still count a released modifier as held, so they are set here to the
  // stroke's own modifiers alone.
  CGEventSetFlags(down, flags);
  CGEventSetFlags(up, flags);
  CGEventPost(kCGHIDEventTap, down);
  CGEventPost(kCGHIDEventTap, up);
  CFRelease(down);
  CFRelease(up);
  return {};
}

}  // namespace robot::mac
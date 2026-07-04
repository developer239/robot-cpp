#pragma once

#include "robot/backend/IKeyboardBackend.h"

namespace robot::win {

// SendInput keyboard injection. Physical keys are injected by scan code
// (KEYEVENTF_SCANCODE, plus KEYEVENTF_EXTENDEDKEY for extended keys), making them
// layout-independent by position. typeUnicode uses KEYEVENTF_UNICODE to deliver a
// character by value regardless of the active layout - the correct replacement
// for the old VkKeyScan path, which was layout-dependent and dropped the Shift
// state so it could not even produce capitals reliably.
//
// This backend is stateless: SendInput carries no cross-call modifier state that
// needs tracking, and chords are built by the portable facade pressing physical
// modifier scan codes around the main key.
class WinKeyboardBackend final : public backend::IKeyboardBackend {
 public:
  std::expected<void, Error> keyDown(Key key) override;
  std::expected<void, Error> keyUp(Key key) override;
  std::expected<void, Error> typeUnicode(char32_t codepoint) override;
};

}  // namespace robot::win
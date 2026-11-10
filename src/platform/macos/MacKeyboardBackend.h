#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <set>

#include "robot/Key.h"
#include "robot/backend/IKeyboardBackend.h"

namespace robot::mac {

// Quartz keyboard injection. Holds a private event source and the set of
// currently-held modifier keys so that every synthesized event carries the
// correct CGEvent modifier flags (the fix for the Command+C / capitals bug).
// typeUnicode injects text directly via CGEventKeyboardSetUnicodeString, on the
// key the current layout types the character with (MacKeyLayout), so its keydown
// and keyup name the same key.
class MacKeyboardBackend final : public backend::IKeyboardBackend {
 public:
  MacKeyboardBackend();
  ~MacKeyboardBackend() override;

  MacKeyboardBackend(const MacKeyboardBackend&) = delete;
  MacKeyboardBackend& operator=(const MacKeyboardBackend&) = delete;

  std::expected<void, Error> keyDown(Key key) override;
  std::expected<void, Error> keyUp(Key key) override;
  std::expected<void, Error> typeUnicode(char32_t codepoint) override;

 private:
  std::expected<void, Error> postKey(Key key, bool down);
  [[nodiscard]] CGEventFlags currentFlags() const;

  CGEventSourceRef source_ = nullptr;
  std::set<Key> heldModifiers_;
};

}  // namespace robot::mac
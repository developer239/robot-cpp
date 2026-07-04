#include "WinKeyMap.h"

#include <array>
#include <unordered_map>
#include <utility>

namespace {

using robot::Key;
using robot::win::ScanCode;

// Set 1 scan codes for the standard PC/AT keyboard. `true` marks extended keys,
// which the E0 prefix distinguishes on real hardware and which SendInput
// reproduces via KEYEVENTF_EXTENDEDKEY.
struct Row {
  Key key;
  WORD code;
  bool extended;
};

constexpr auto kTable = std::to_array<Row>({
    // Letters (positional).
    {Key::A, 0x1E, false}, {Key::B, 0x30, false}, {Key::C, 0x2E, false},
    {Key::D, 0x20, false}, {Key::E, 0x12, false}, {Key::F, 0x21, false},
    {Key::G, 0x22, false}, {Key::H, 0x23, false}, {Key::I, 0x17, false},
    {Key::J, 0x24, false}, {Key::K, 0x25, false}, {Key::L, 0x26, false},
    {Key::M, 0x32, false}, {Key::N, 0x31, false}, {Key::O, 0x18, false},
    {Key::P, 0x19, false}, {Key::Q, 0x10, false}, {Key::R, 0x13, false},
    {Key::S, 0x1F, false}, {Key::T, 0x14, false}, {Key::U, 0x16, false},
    {Key::V, 0x2F, false}, {Key::W, 0x11, false}, {Key::X, 0x2D, false},
    {Key::Y, 0x15, false}, {Key::Z, 0x2C, false},
    // Digit row.
    {Key::Digit1, 0x02, false}, {Key::Digit2, 0x03, false},
    {Key::Digit3, 0x04, false}, {Key::Digit4, 0x05, false},
    {Key::Digit5, 0x06, false}, {Key::Digit6, 0x07, false},
    {Key::Digit7, 0x08, false}, {Key::Digit8, 0x09, false},
    {Key::Digit9, 0x0A, false}, {Key::Digit0, 0x0B, false},
    // Whitespace and editing.
    {Key::Enter, 0x1C, false}, {Key::Escape, 0x01, false},
    {Key::Backspace, 0x0E, false}, {Key::Tab, 0x0F, false},
    {Key::Space, 0x39, false},
    // Punctuation.
    {Key::Minus, 0x0C, false}, {Key::Equal, 0x0D, false},
    {Key::LeftBracket, 0x1A, false}, {Key::RightBracket, 0x1B, false},
    {Key::Backslash, 0x2B, false}, {Key::Semicolon, 0x27, false},
    {Key::Quote, 0x28, false}, {Key::Grave, 0x29, false},
    {Key::Comma, 0x33, false}, {Key::Period, 0x34, false},
    {Key::Slash, 0x35, false}, {Key::CapsLock, 0x3A, false},
    // Function row.
    {Key::F1, 0x3B, false}, {Key::F2, 0x3C, false}, {Key::F3, 0x3D, false},
    {Key::F4, 0x3E, false}, {Key::F5, 0x3F, false}, {Key::F6, 0x40, false},
    {Key::F7, 0x41, false}, {Key::F8, 0x42, false}, {Key::F9, 0x43, false},
    {Key::F10, 0x44, false}, {Key::F11, 0x57, false}, {Key::F12, 0x58, false},
    // Navigation cluster (extended).
    {Key::PrintScreen, 0x37, true}, {Key::ScrollLock, 0x46, false},
    {Key::Pause, 0x45, false}, {Key::Insert, 0x52, true},
    {Key::Home, 0x47, true}, {Key::PageUp, 0x49, true},
    {Key::Delete, 0x53, true}, {Key::End, 0x4F, true},
    {Key::PageDown, 0x51, true}, {Key::RightArrow, 0x4D, true},
    {Key::LeftArrow, 0x4B, true}, {Key::DownArrow, 0x50, true},
    {Key::UpArrow, 0x48, true},
    // Keypad. NumLock and KeypadDivide/Enter are extended.
    {Key::NumLock, 0x45, true}, {Key::KeypadDivide, 0x35, true},
    {Key::KeypadMultiply, 0x37, false}, {Key::KeypadMinus, 0x4A, false},
    {Key::KeypadPlus, 0x4E, false}, {Key::KeypadEnter, 0x1C, true},
    {Key::Keypad1, 0x4F, false}, {Key::Keypad2, 0x50, false},
    {Key::Keypad3, 0x51, false}, {Key::Keypad4, 0x4B, false},
    {Key::Keypad5, 0x4C, false}, {Key::Keypad6, 0x4D, false},
    {Key::Keypad7, 0x47, false}, {Key::Keypad8, 0x48, false},
    {Key::Keypad9, 0x49, false}, {Key::Keypad0, 0x52, false},
    {Key::KeypadDecimal, 0x53, false},
    // Extra / international.
    {Key::NonUsBackslash, 0x56, false}, {Key::Application, 0x5D, true},
    {Key::IntlRo, 0x73, false}, {Key::IntlYen, 0x7D, false},
    {Key::Lang1, 0x72, false}, {Key::Lang2, 0x71, false},
    // Modifiers. Right-hand Control/Alt and both Meta keys are extended.
    {Key::LeftControl, 0x1D, false}, {Key::LeftShift, 0x2A, false},
    {Key::LeftAlt, 0x38, false}, {Key::LeftMeta, 0x5B, true},
    {Key::RightControl, 0x1D, true}, {Key::RightShift, 0x36, false},
    {Key::RightAlt, 0x38, true}, {Key::RightMeta, 0x5C, true},
});

}  // namespace

namespace robot::win {

ScanCode keyToScanCode(const Key key) {
  static const std::unordered_map<Key, ScanCode> map = [] {
    std::unordered_map<Key, ScanCode> m;
    m.reserve(kTable.size());
    for (const auto& r : kTable) m.emplace(r.key, ScanCode{r.code, r.extended, true});
    return m;
  }();
  const auto it = map.find(key);
  if (it == map.end()) return ScanCode{};
  return it->second;
}

Key scanCodeToKey(const WORD scanCode, const bool extended) {
  // Key the reverse lookup on (code, extended): several keys share a scan code
  // and differ only by the extended flag (KeypadEnter vs Enter, RightAlt vs
  // LeftAlt, KeypadDivide vs Slash), so the flag disambiguates them.
  static const std::unordered_map<std::uint32_t, Key> map = [] {
    std::unordered_map<std::uint32_t, Key> m;
    m.reserve(kTable.size());
    for (const auto& r : kTable) {
      const std::uint32_t k =
          (static_cast<std::uint32_t>(r.code) << 1) | (r.extended ? 1u : 0u);
      m.emplace(k, r.key);
    }
    return m;
  }();
  const std::uint32_t k =
      (static_cast<std::uint32_t>(scanCode) << 1) | (extended ? 1u : 0u);
  const auto it = map.find(k);
  if (it == map.end()) return Key::Unknown;
  return it->second;
}

}  // namespace robot::win

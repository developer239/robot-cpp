#include "MacKeyMap.h"

#include <array>
#include <unordered_map>
#include <utility>

// macOS virtual key codes (from HIToolbox Events.h). Defined here rather than
// pulling in the deprecated Carbon framework; these are stable ABI values.
namespace {

enum : CGKeyCode {
  kVK_A = 0x00, kVK_S = 0x01, kVK_D = 0x02, kVK_F = 0x03, kVK_H = 0x04,
  kVK_G = 0x05, kVK_Z = 0x06, kVK_X = 0x07, kVK_C = 0x08, kVK_V = 0x09,
  kVK_ISO_Section = 0x0A, kVK_B = 0x0B, kVK_Q = 0x0C, kVK_W = 0x0D,
  kVK_E = 0x0E, kVK_R = 0x0F, kVK_Y = 0x10, kVK_T = 0x11, kVK_1 = 0x12,
  kVK_2 = 0x13, kVK_3 = 0x14, kVK_4 = 0x15, kVK_6 = 0x16, kVK_5 = 0x17,
  kVK_Equal = 0x18, kVK_9 = 0x19, kVK_7 = 0x1A, kVK_Minus = 0x1B,
  kVK_8 = 0x1C, kVK_0 = 0x1D, kVK_RightBracket = 0x1E, kVK_O = 0x1F,
  kVK_U = 0x20, kVK_LeftBracket = 0x21, kVK_I = 0x22, kVK_P = 0x23,
  kVK_Return = 0x24, kVK_L = 0x25, kVK_J = 0x26, kVK_Quote = 0x27,
  kVK_K = 0x28, kVK_Semicolon = 0x29, kVK_Backslash = 0x2A, kVK_Comma = 0x2B,
  kVK_Slash = 0x2C, kVK_N = 0x2D, kVK_M = 0x2E, kVK_Period = 0x2F,
  kVK_Tab = 0x30, kVK_Space = 0x31, kVK_Grave = 0x32, kVK_Delete = 0x33,
  kVK_Escape = 0x35, kVK_RightCommand = 0x36, kVK_Command = 0x37,
  kVK_Shift = 0x38, kVK_CapsLock = 0x39, kVK_Option = 0x3A, kVK_Control = 0x3B,
  kVK_RightShift = 0x3C, kVK_RightOption = 0x3D, kVK_RightControl = 0x3E,
  kVK_F17 = 0x40, kVK_KeypadDecimal = 0x41, kVK_KeypadMultiply = 0x43,
  kVK_KeypadPlus = 0x45, kVK_KeypadClear = 0x47, kVK_KeypadDivide = 0x4B,
  kVK_KeypadEnter = 0x4C, kVK_KeypadMinus = 0x4E, kVK_F18 = 0x4F,
  kVK_F19 = 0x50, kVK_KeypadEquals = 0x51, kVK_Keypad0 = 0x52,
  kVK_Keypad1 = 0x53, kVK_Keypad2 = 0x54, kVK_Keypad3 = 0x55,
  kVK_Keypad4 = 0x56, kVK_Keypad5 = 0x57, kVK_Keypad6 = 0x58,
  kVK_Keypad7 = 0x59, kVK_F20 = 0x5A, kVK_Keypad8 = 0x5B, kVK_Keypad9 = 0x5C,
  kVK_JIS_Yen = 0x5D, kVK_JIS_Underscore = 0x5E, kVK_JIS_Eisu = 0x66,
  kVK_JIS_Kana = 0x68, kVK_F5 = 0x60, kVK_F6 = 0x61, kVK_F7 = 0x62,
  kVK_F3 = 0x63, kVK_F8 = 0x64, kVK_F9 = 0x65, kVK_F11 = 0x67, kVK_F13 = 0x69,
  kVK_F16 = 0x6A, kVK_F14 = 0x6B, kVK_F10 = 0x6D, kVK_F12 = 0x6F,
  kVK_F15 = 0x71, kVK_Help = 0x72, kVK_Home = 0x73, kVK_PageUp = 0x74,
  kVK_ForwardDelete = 0x75, kVK_F4 = 0x76, kVK_End = 0x77, kVK_F2 = 0x78,
  kVK_PageDown = 0x79, kVK_F1 = 0x7A, kVK_LeftArrow = 0x7B,
  kVK_RightArrow = 0x7C, kVK_DownArrow = 0x7D, kVK_UpArrow = 0x7E,
};

using robot::Key;

// Single source of truth for both directions, kept in sync by construction.
constexpr auto kTable = std::to_array<std::pair<Key, CGKeyCode>>({
    {Key::A, kVK_A}, {Key::B, kVK_B}, {Key::C, kVK_C}, {Key::D, kVK_D},
    {Key::E, kVK_E}, {Key::F, kVK_F}, {Key::G, kVK_G}, {Key::H, kVK_H},
    {Key::I, kVK_I}, {Key::J, kVK_J}, {Key::K, kVK_K}, {Key::L, kVK_L},
    {Key::M, kVK_M}, {Key::N, kVK_N}, {Key::O, kVK_O}, {Key::P, kVK_P},
    {Key::Q, kVK_Q}, {Key::R, kVK_R}, {Key::S, kVK_S}, {Key::T, kVK_T},
    {Key::U, kVK_U}, {Key::V, kVK_V}, {Key::W, kVK_W}, {Key::X, kVK_X},
    {Key::Y, kVK_Y}, {Key::Z, kVK_Z},
    {Key::Digit1, kVK_1}, {Key::Digit2, kVK_2}, {Key::Digit3, kVK_3},
    {Key::Digit4, kVK_4}, {Key::Digit5, kVK_5}, {Key::Digit6, kVK_6},
    {Key::Digit7, kVK_7}, {Key::Digit8, kVK_8}, {Key::Digit9, kVK_9},
    {Key::Digit0, kVK_0},
    {Key::Enter, kVK_Return}, {Key::Escape, kVK_Escape},
    {Key::Backspace, kVK_Delete}, {Key::Tab, kVK_Tab}, {Key::Space, kVK_Space},
    {Key::Minus, kVK_Minus}, {Key::Equal, kVK_Equal},
    {Key::LeftBracket, kVK_LeftBracket}, {Key::RightBracket, kVK_RightBracket},
    {Key::Backslash, kVK_Backslash}, {Key::Semicolon, kVK_Semicolon},
    {Key::Quote, kVK_Quote}, {Key::Grave, kVK_Grave}, {Key::Comma, kVK_Comma},
    {Key::Period, kVK_Period}, {Key::Slash, kVK_Slash},
    {Key::CapsLock, kVK_CapsLock},
    {Key::F1, kVK_F1}, {Key::F2, kVK_F2}, {Key::F3, kVK_F3}, {Key::F4, kVK_F4},
    {Key::F5, kVK_F5}, {Key::F6, kVK_F6}, {Key::F7, kVK_F7}, {Key::F8, kVK_F8},
    {Key::F9, kVK_F9}, {Key::F10, kVK_F10}, {Key::F11, kVK_F11},
    {Key::F12, kVK_F12}, {Key::F13, kVK_F13}, {Key::F14, kVK_F14},
    {Key::F15, kVK_F15}, {Key::F16, kVK_F16}, {Key::F17, kVK_F17},
    {Key::F18, kVK_F18}, {Key::F19, kVK_F19}, {Key::F20, kVK_F20},
    {Key::Home, kVK_Home}, {Key::PageUp, kVK_PageUp},
    {Key::Delete, kVK_ForwardDelete}, {Key::End, kVK_End},
    {Key::PageDown, kVK_PageDown}, {Key::Help, kVK_Help},
    {Key::RightArrow, kVK_RightArrow}, {Key::LeftArrow, kVK_LeftArrow},
    {Key::DownArrow, kVK_DownArrow}, {Key::UpArrow, kVK_UpArrow},
    {Key::NumLock, kVK_KeypadClear}, {Key::KeypadDivide, kVK_KeypadDivide},
    {Key::KeypadMultiply, kVK_KeypadMultiply},
    {Key::KeypadMinus, kVK_KeypadMinus}, {Key::KeypadPlus, kVK_KeypadPlus},
    {Key::KeypadEnter, kVK_KeypadEnter}, {Key::Keypad1, kVK_Keypad1},
    {Key::Keypad2, kVK_Keypad2}, {Key::Keypad3, kVK_Keypad3},
    {Key::Keypad4, kVK_Keypad4}, {Key::Keypad5, kVK_Keypad5},
    {Key::Keypad6, kVK_Keypad6}, {Key::Keypad7, kVK_Keypad7},
    {Key::Keypad8, kVK_Keypad8}, {Key::Keypad9, kVK_Keypad9},
    {Key::Keypad0, kVK_Keypad0}, {Key::KeypadDecimal, kVK_KeypadDecimal},
    {Key::KeypadEqual, kVK_KeypadEquals},
    {Key::NonUsBackslash, kVK_ISO_Section}, {Key::IntlYen, kVK_JIS_Yen},
    {Key::IntlRo, kVK_JIS_Underscore}, {Key::Lang1, kVK_JIS_Kana},
    {Key::Lang2, kVK_JIS_Eisu},
    {Key::LeftControl, kVK_Control}, {Key::LeftShift, kVK_Shift},
    {Key::LeftAlt, kVK_Option}, {Key::LeftMeta, kVK_Command},
    {Key::RightControl, kVK_RightControl}, {Key::RightShift, kVK_RightShift},
    {Key::RightAlt, kVK_RightOption}, {Key::RightMeta, kVK_RightCommand},
});

}  // namespace

namespace robot::mac {

std::optional<CGKeyCode> keyToMacKeycode(const Key key) {
  static const std::unordered_map<Key, CGKeyCode> map = [] {
    std::unordered_map<Key, CGKeyCode> m;
    m.reserve(kTable.size());
    for (const auto& [k, code] : kTable) m.emplace(k, code);
    return m;
  }();
  const auto it = map.find(key);
  if (it == map.end()) return std::nullopt;
  return it->second;
}

Key macKeycodeToKey(const CGKeyCode keycode) {
  static const std::unordered_map<CGKeyCode, Key> map = [] {
    std::unordered_map<CGKeyCode, Key> m;
    m.reserve(kTable.size());
    for (const auto& [k, code] : kTable) m.emplace(code, k);
    return m;
  }();
  const auto it = map.find(keycode);
  if (it == map.end()) return Key::Unknown;
  return it->second;
}

CGEventFlags macModifierFlag(const Key key) {
  switch (key) {
    case Key::LeftShift:
    case Key::RightShift: return kCGEventFlagMaskShift;
    case Key::LeftControl:
    case Key::RightControl: return kCGEventFlagMaskControl;
    case Key::LeftAlt:
    case Key::RightAlt: return kCGEventFlagMaskAlternate;
    case Key::LeftMeta:
    case Key::RightMeta: return kCGEventFlagMaskCommand;
    case Key::CapsLock: return kCGEventFlagMaskAlphaShift;
    default: return 0;
  }
}

bool isModifierKey(const Key key) { return macModifierFlag(key) != 0; }

}  // namespace robot::mac

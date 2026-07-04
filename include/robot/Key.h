#pragma once

#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace robot {

// Physical keyboard keys, identified by position, not by the character they
// produce. This is the single most important distinction in the keyboard API:
//
//   * A Key names a physical key by its location, independent of the active
//     layout. Key::A is "the key in the US-QWERTY-A position"; on an AZERTY
//     layout that same physical key produces 'q'. Use Key for shortcuts, games,
//     and anything positional (WASD movement, Control+key chords).
//
//   * To produce specific characters or text (accented letters, symbols,
//     non-Latin scripts), use Keyboard::typeText / typeChar, which inject
//     Unicode directly and are layout-independent by construction. Do not try to
//     spell out characters by pressing Keys; that only works on the one layout
//     you hard-coded for.
//
// Each enumerator's underlying value is its USB HID Keyboard/Keypad usage id
// (Usage Page 0x07), a stable cross-platform key identity every backend maps to
// its native code (CGKeyCode on macOS, scan code on Windows, evdev keycode on
// Linux). Consumer/media keys live on a different HID page and are intentionally
// excluded to keep this enum coherent.
enum class Key : std::uint16_t {
  Unknown = 0x00,

  // Letters (US QWERTY position).
  A = 0x04, B = 0x05, C = 0x06, D = 0x07, E = 0x08, F = 0x09, G = 0x0A,
  H = 0x0B, I = 0x0C, J = 0x0D, K = 0x0E, L = 0x0F, M = 0x10, N = 0x11,
  O = 0x12, P = 0x13, Q = 0x14, R = 0x15, S = 0x16, T = 0x17, U = 0x18,
  V = 0x19, W = 0x1A, X = 0x1B, Y = 0x1C, Z = 0x1D,

  // Top-row digits.
  Digit1 = 0x1E, Digit2 = 0x1F, Digit3 = 0x20, Digit4 = 0x21, Digit5 = 0x22,
  Digit6 = 0x23, Digit7 = 0x24, Digit8 = 0x25, Digit9 = 0x26, Digit0 = 0x27,

  // Editing and whitespace.
  Enter = 0x28, Escape = 0x29, Backspace = 0x2A, Tab = 0x2B, Space = 0x2C,

  // Punctuation (US QWERTY positions).
  Minus = 0x2D, Equal = 0x2E, LeftBracket = 0x2F, RightBracket = 0x30,
  Backslash = 0x31, NonUsHash = 0x32, Semicolon = 0x33, Quote = 0x34,
  Grave = 0x35, Comma = 0x36, Period = 0x37, Slash = 0x38, CapsLock = 0x39,

  // Function row.
  F1 = 0x3A, F2 = 0x3B, F3 = 0x3C, F4 = 0x3D, F5 = 0x3E, F6 = 0x3F,
  F7 = 0x40, F8 = 0x41, F9 = 0x42, F10 = 0x43, F11 = 0x44, F12 = 0x45,

  // Navigation cluster and neighbours.
  PrintScreen = 0x46, ScrollLock = 0x47, Pause = 0x48, Insert = 0x49,
  Home = 0x4A, PageUp = 0x4B, Delete = 0x4C, End = 0x4D, PageDown = 0x4E,
  RightArrow = 0x4F, LeftArrow = 0x50, DownArrow = 0x51, UpArrow = 0x52,

  // Keypad.
  NumLock = 0x53, KeypadDivide = 0x54, KeypadMultiply = 0x55,
  KeypadMinus = 0x56, KeypadPlus = 0x57, KeypadEnter = 0x58, Keypad1 = 0x59,
  Keypad2 = 0x5A, Keypad3 = 0x5B, Keypad4 = 0x5C, Keypad5 = 0x5D,
  Keypad6 = 0x5E, Keypad7 = 0x5F, Keypad8 = 0x60, Keypad9 = 0x61,
  Keypad0 = 0x62, KeypadDecimal = 0x63, KeypadEqual = 0x67,

  // Extra and international physical keys.
  NonUsBackslash = 0x64, Application = 0x65, Power = 0x66,

  // Extended function row.
  F13 = 0x68, F14 = 0x69, F15 = 0x6A, F16 = 0x6B, F17 = 0x6C, F18 = 0x6D,
  F19 = 0x6E, F20 = 0x6F, F21 = 0x70, F22 = 0x71, F23 = 0x72, F24 = 0x73,

  Help = 0x75, Menu = 0x76,

  IntlRo = 0x87, IntlYen = 0x89, Lang1 = 0x90, Lang2 = 0x91,

  // Modifier keys are distinct physical keys; left vs right matters. The logical
  // Modifier set (Modifiers.h) is a separate concept for building chords.
  LeftControl = 0xE0, LeftShift = 0xE1, LeftAlt = 0xE2, LeftMeta = 0xE3,
  RightControl = 0xE4, RightShift = 0xE5, RightAlt = 0xE6, RightMeta = 0xE7,
};

// The USB HID usage id backing a Key (Usage Page 0x07). Backends translate this
// to their native code.
[[nodiscard]] constexpr std::uint16_t keyToHidUsage(const Key key) {
  return static_cast<std::uint16_t>(key);
}

constexpr std::string_view toString(const Key key) {
  switch (key) {
    case Key::Unknown: return "Unknown";
    case Key::A: return "A";
    case Key::B: return "B";
    case Key::C: return "C";
    case Key::D: return "D";
    case Key::E: return "E";
    case Key::F: return "F";
    case Key::G: return "G";
    case Key::H: return "H";
    case Key::I: return "I";
    case Key::J: return "J";
    case Key::K: return "K";
    case Key::L: return "L";
    case Key::M: return "M";
    case Key::N: return "N";
    case Key::O: return "O";
    case Key::P: return "P";
    case Key::Q: return "Q";
    case Key::R: return "R";
    case Key::S: return "S";
    case Key::T: return "T";
    case Key::U: return "U";
    case Key::V: return "V";
    case Key::W: return "W";
    case Key::X: return "X";
    case Key::Y: return "Y";
    case Key::Z: return "Z";
    case Key::Digit1: return "Digit1";
    case Key::Digit2: return "Digit2";
    case Key::Digit3: return "Digit3";
    case Key::Digit4: return "Digit4";
    case Key::Digit5: return "Digit5";
    case Key::Digit6: return "Digit6";
    case Key::Digit7: return "Digit7";
    case Key::Digit8: return "Digit8";
    case Key::Digit9: return "Digit9";
    case Key::Digit0: return "Digit0";
    case Key::Enter: return "Enter";
    case Key::Escape: return "Escape";
    case Key::Backspace: return "Backspace";
    case Key::Tab: return "Tab";
    case Key::Space: return "Space";
    case Key::Minus: return "Minus";
    case Key::Equal: return "Equal";
    case Key::LeftBracket: return "LeftBracket";
    case Key::RightBracket: return "RightBracket";
    case Key::Backslash: return "Backslash";
    case Key::NonUsHash: return "NonUsHash";
    case Key::Semicolon: return "Semicolon";
    case Key::Quote: return "Quote";
    case Key::Grave: return "Grave";
    case Key::Comma: return "Comma";
    case Key::Period: return "Period";
    case Key::Slash: return "Slash";
    case Key::CapsLock: return "CapsLock";
    case Key::F1: return "F1";
    case Key::F2: return "F2";
    case Key::F3: return "F3";
    case Key::F4: return "F4";
    case Key::F5: return "F5";
    case Key::F6: return "F6";
    case Key::F7: return "F7";
    case Key::F8: return "F8";
    case Key::F9: return "F9";
    case Key::F10: return "F10";
    case Key::F11: return "F11";
    case Key::F12: return "F12";
    case Key::PrintScreen: return "PrintScreen";
    case Key::ScrollLock: return "ScrollLock";
    case Key::Pause: return "Pause";
    case Key::Insert: return "Insert";
    case Key::Home: return "Home";
    case Key::PageUp: return "PageUp";
    case Key::Delete: return "Delete";
    case Key::End: return "End";
    case Key::PageDown: return "PageDown";
    case Key::RightArrow: return "RightArrow";
    case Key::LeftArrow: return "LeftArrow";
    case Key::DownArrow: return "DownArrow";
    case Key::UpArrow: return "UpArrow";
    case Key::NumLock: return "NumLock";
    case Key::KeypadDivide: return "KeypadDivide";
    case Key::KeypadMultiply: return "KeypadMultiply";
    case Key::KeypadMinus: return "KeypadMinus";
    case Key::KeypadPlus: return "KeypadPlus";
    case Key::KeypadEnter: return "KeypadEnter";
    case Key::Keypad1: return "Keypad1";
    case Key::Keypad2: return "Keypad2";
    case Key::Keypad3: return "Keypad3";
    case Key::Keypad4: return "Keypad4";
    case Key::Keypad5: return "Keypad5";
    case Key::Keypad6: return "Keypad6";
    case Key::Keypad7: return "Keypad7";
    case Key::Keypad8: return "Keypad8";
    case Key::Keypad9: return "Keypad9";
    case Key::Keypad0: return "Keypad0";
    case Key::KeypadDecimal: return "KeypadDecimal";
    case Key::KeypadEqual: return "KeypadEqual";
    case Key::NonUsBackslash: return "NonUsBackslash";
    case Key::Application: return "Application";
    case Key::Power: return "Power";
    case Key::F13: return "F13";
    case Key::F14: return "F14";
    case Key::F15: return "F15";
    case Key::F16: return "F16";
    case Key::F17: return "F17";
    case Key::F18: return "F18";
    case Key::F19: return "F19";
    case Key::F20: return "F20";
    case Key::F21: return "F21";
    case Key::F22: return "F22";
    case Key::F23: return "F23";
    case Key::F24: return "F24";
    case Key::Help: return "Help";
    case Key::Menu: return "Menu";
    case Key::IntlRo: return "IntlRo";
    case Key::IntlYen: return "IntlYen";
    case Key::Lang1: return "Lang1";
    case Key::Lang2: return "Lang2";
    case Key::LeftControl: return "LeftControl";
    case Key::LeftShift: return "LeftShift";
    case Key::LeftAlt: return "LeftAlt";
    case Key::LeftMeta: return "LeftMeta";
    case Key::RightControl: return "RightControl";
    case Key::RightShift: return "RightShift";
    case Key::RightAlt: return "RightAlt";
    case Key::RightMeta: return "RightMeta";
  }
  std::abort();
}

}  // namespace robot

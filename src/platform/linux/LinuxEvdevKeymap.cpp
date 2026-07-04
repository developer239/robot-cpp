#include "LinuxEvdevKeymap.h"

#include <linux/input-event-codes.h>

#include <array>
#include <unordered_map>
#include <utility>

namespace {

using robot::Key;

constexpr std::array<std::pair<Key, std::uint16_t>, 100> kTable{{
    {Key::A, KEY_A}, {Key::B, KEY_B}, {Key::C, KEY_C}, {Key::D, KEY_D},
    {Key::E, KEY_E}, {Key::F, KEY_F}, {Key::G, KEY_G}, {Key::H, KEY_H},
    {Key::I, KEY_I}, {Key::J, KEY_J}, {Key::K, KEY_K}, {Key::L, KEY_L},
    {Key::M, KEY_M}, {Key::N, KEY_N}, {Key::O, KEY_O}, {Key::P, KEY_P},
    {Key::Q, KEY_Q}, {Key::R, KEY_R}, {Key::S, KEY_S}, {Key::T, KEY_T},
    {Key::U, KEY_U}, {Key::V, KEY_V}, {Key::W, KEY_W}, {Key::X, KEY_X},
    {Key::Y, KEY_Y}, {Key::Z, KEY_Z},
    {Key::Digit1, KEY_1}, {Key::Digit2, KEY_2}, {Key::Digit3, KEY_3},
    {Key::Digit4, KEY_4}, {Key::Digit5, KEY_5}, {Key::Digit6, KEY_6},
    {Key::Digit7, KEY_7}, {Key::Digit8, KEY_8}, {Key::Digit9, KEY_9},
    {Key::Digit0, KEY_0},
    {Key::Enter, KEY_ENTER}, {Key::Escape, KEY_ESC},
    {Key::Backspace, KEY_BACKSPACE}, {Key::Tab, KEY_TAB},
    {Key::Space, KEY_SPACE}, {Key::Minus, KEY_MINUS}, {Key::Equal, KEY_EQUAL},
    {Key::LeftBracket, KEY_LEFTBRACE}, {Key::RightBracket, KEY_RIGHTBRACE},
    {Key::Backslash, KEY_BACKSLASH}, {Key::Semicolon, KEY_SEMICOLON},
    {Key::Quote, KEY_APOSTROPHE}, {Key::Grave, KEY_GRAVE},
    {Key::Comma, KEY_COMMA}, {Key::Period, KEY_DOT}, {Key::Slash, KEY_SLASH},
    {Key::CapsLock, KEY_CAPSLOCK},
    {Key::F1, KEY_F1}, {Key::F2, KEY_F2}, {Key::F3, KEY_F3}, {Key::F4, KEY_F4},
    {Key::F5, KEY_F5}, {Key::F6, KEY_F6}, {Key::F7, KEY_F7}, {Key::F8, KEY_F8},
    {Key::F9, KEY_F9}, {Key::F10, KEY_F10}, {Key::F11, KEY_F11},
    {Key::F12, KEY_F12},
    {Key::PrintScreen, KEY_SYSRQ}, {Key::ScrollLock, KEY_SCROLLLOCK},
    {Key::Pause, KEY_PAUSE}, {Key::Insert, KEY_INSERT}, {Key::Home, KEY_HOME},
    {Key::PageUp, KEY_PAGEUP}, {Key::Delete, KEY_DELETE}, {Key::End, KEY_END},
    {Key::PageDown, KEY_PAGEDOWN}, {Key::RightArrow, KEY_RIGHT},
    {Key::LeftArrow, KEY_LEFT}, {Key::DownArrow, KEY_DOWN},
    {Key::UpArrow, KEY_UP},
    {Key::NumLock, KEY_NUMLOCK}, {Key::KeypadDivide, KEY_KPSLASH},
    {Key::KeypadMultiply, KEY_KPASTERISK}, {Key::KeypadMinus, KEY_KPMINUS},
    {Key::KeypadPlus, KEY_KPPLUS}, {Key::KeypadEnter, KEY_KPENTER},
    {Key::Keypad1, KEY_KP1}, {Key::Keypad2, KEY_KP2}, {Key::Keypad3, KEY_KP3},
    {Key::Keypad4, KEY_KP4}, {Key::Keypad5, KEY_KP5}, {Key::Keypad6, KEY_KP6},
    {Key::Keypad7, KEY_KP7}, {Key::Keypad8, KEY_KP8}, {Key::Keypad9, KEY_KP9},
    {Key::Keypad0, KEY_KP0}, {Key::KeypadDecimal, KEY_KPDOT},
    {Key::Application, KEY_COMPOSE},
    {Key::LeftControl, KEY_LEFTCTRL}, {Key::LeftShift, KEY_LEFTSHIFT},
    {Key::LeftAlt, KEY_LEFTALT}, {Key::LeftMeta, KEY_LEFTMETA},
    {Key::RightControl, KEY_RIGHTCTRL}, {Key::RightShift, KEY_RIGHTSHIFT},
    {Key::RightAlt, KEY_RIGHTALT}, {Key::RightMeta, KEY_RIGHTMETA},
}};

}  // namespace

namespace robot::linux_evdev {

std::optional<std::uint16_t> keyToEvdev(const Key key) {
  static const std::unordered_map<Key, std::uint16_t> map = [] {
    std::unordered_map<Key, std::uint16_t> m;
    m.reserve(kTable.size());
    for (const auto& [k, code] : kTable) m.emplace(k, code);
    return m;
  }();
  const auto it = map.find(key);
  if (it == map.end()) return std::nullopt;
  return it->second;
}

}  // namespace robot::linux_evdev
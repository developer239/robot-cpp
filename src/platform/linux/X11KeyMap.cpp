#include "X11KeyMap.h"

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include <array>
#include <unordered_map>
#include <utility>

namespace {

using robot::Key;

// Position-based Keys mapped to the keysym that, on a US/Latin layout, sits on
// that physical key. The backend resolves keysym -> current keycode via the
// server, so this drives the physical position for those layouts; genuinely
// layout-independent character output is the separate typeUnicode path.
constexpr auto kTable = std::to_array<std::pair<Key, KeySym>>({
    {Key::A, XK_a}, {Key::B, XK_b}, {Key::C, XK_c}, {Key::D, XK_d},
    {Key::E, XK_e}, {Key::F, XK_f}, {Key::G, XK_g}, {Key::H, XK_h},
    {Key::I, XK_i}, {Key::J, XK_j}, {Key::K, XK_k}, {Key::L, XK_l},
    {Key::M, XK_m}, {Key::N, XK_n}, {Key::O, XK_o}, {Key::P, XK_p},
    {Key::Q, XK_q}, {Key::R, XK_r}, {Key::S, XK_s}, {Key::T, XK_t},
    {Key::U, XK_u}, {Key::V, XK_v}, {Key::W, XK_w}, {Key::X, XK_x},
    {Key::Y, XK_y}, {Key::Z, XK_z},
    {Key::Digit1, XK_1}, {Key::Digit2, XK_2}, {Key::Digit3, XK_3},
    {Key::Digit4, XK_4}, {Key::Digit5, XK_5}, {Key::Digit6, XK_6},
    {Key::Digit7, XK_7}, {Key::Digit8, XK_8}, {Key::Digit9, XK_9},
    {Key::Digit0, XK_0},
    {Key::Enter, XK_Return}, {Key::Escape, XK_Escape},
    {Key::Backspace, XK_BackSpace}, {Key::Tab, XK_Tab}, {Key::Space, XK_space},
    {Key::Minus, XK_minus}, {Key::Equal, XK_equal},
    {Key::LeftBracket, XK_bracketleft}, {Key::RightBracket, XK_bracketright},
    {Key::Backslash, XK_backslash}, {Key::Semicolon, XK_semicolon},
    {Key::Quote, XK_apostrophe}, {Key::Grave, XK_grave},
    {Key::Comma, XK_comma}, {Key::Period, XK_period}, {Key::Slash, XK_slash},
    {Key::CapsLock, XK_Caps_Lock},
    {Key::F1, XK_F1}, {Key::F2, XK_F2}, {Key::F3, XK_F3}, {Key::F4, XK_F4},
    {Key::F5, XK_F5}, {Key::F6, XK_F6}, {Key::F7, XK_F7}, {Key::F8, XK_F8},
    {Key::F9, XK_F9}, {Key::F10, XK_F10}, {Key::F11, XK_F11},
    {Key::F12, XK_F12}, {Key::F13, XK_F13}, {Key::F14, XK_F14},
    {Key::F15, XK_F15}, {Key::F16, XK_F16},
    {Key::PrintScreen, XK_Print}, {Key::ScrollLock, XK_Scroll_Lock},
    {Key::Pause, XK_Pause}, {Key::Insert, XK_Insert}, {Key::Home, XK_Home},
    {Key::PageUp, XK_Prior}, {Key::Delete, XK_Delete}, {Key::End, XK_End},
    {Key::PageDown, XK_Next}, {Key::RightArrow, XK_Right},
    {Key::LeftArrow, XK_Left}, {Key::DownArrow, XK_Down},
    {Key::UpArrow, XK_Up},
    {Key::NumLock, XK_Num_Lock}, {Key::KeypadDivide, XK_KP_Divide},
    {Key::KeypadMultiply, XK_KP_Multiply}, {Key::KeypadMinus, XK_KP_Subtract},
    {Key::KeypadPlus, XK_KP_Add}, {Key::KeypadEnter, XK_KP_Enter},
    {Key::Keypad1, XK_KP_1}, {Key::Keypad2, XK_KP_2}, {Key::Keypad3, XK_KP_3},
    {Key::Keypad4, XK_KP_4}, {Key::Keypad5, XK_KP_5}, {Key::Keypad6, XK_KP_6},
    {Key::Keypad7, XK_KP_7}, {Key::Keypad8, XK_KP_8}, {Key::Keypad9, XK_KP_9},
    {Key::Keypad0, XK_KP_0}, {Key::KeypadDecimal, XK_KP_Decimal},
    {Key::Application, XK_Menu},
    {Key::LeftControl, XK_Control_L}, {Key::LeftShift, XK_Shift_L},
    {Key::LeftAlt, XK_Alt_L}, {Key::LeftMeta, XK_Super_L},
    {Key::RightControl, XK_Control_R}, {Key::RightShift, XK_Shift_R},
    {Key::RightAlt, XK_Alt_R}, {Key::RightMeta, XK_Super_R},
});

}  // namespace

namespace robot::x11 {

KeySym keyToKeysym(const Key key) {
  static const std::unordered_map<Key, KeySym> map = [] {
    std::unordered_map<Key, KeySym> m;
    m.reserve(kTable.size());
    for (const auto& [k, ks] : kTable) m.emplace(k, ks);
    return m;
  }();
  const auto it = map.find(key);
  if (it == map.end()) return NoSymbol;
  return it->second;
}

Key keysymToKey(const KeySym keysym) {
  static const std::unordered_map<KeySym, Key> map = [] {
    std::unordered_map<KeySym, Key> m;
    m.reserve(kTable.size());
    for (const auto& [k, ks] : kTable) m.emplace(ks, k);
    return m;
  }();
  const auto it = map.find(keysym);
  if (it == map.end()) return Key::Unknown;
  return it->second;
}

}  // namespace robot::x11

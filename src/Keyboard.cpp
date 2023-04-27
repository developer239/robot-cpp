#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

#include <iostream>

#include "./Keyboard.h"
#include "./Utils.h"

namespace Robot {

void Keyboard::Type(const std::string &query) {
  for (char c : query) {
    Click(c);
  }
}

void Keyboard::Click(char asciiChar) {
  Press(asciiChar);
  Release(asciiChar);
}

void Keyboard::Press(char asciiChar) {
  KeyCode keycode = AsciiToKeycode(asciiChar);
#ifdef __APPLE__
  CGEventRef event = CGEventCreateKeyboardEvent(nullptr, keycode, true);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  SendInput(1, &input, sizeof(INPUT));
#endif
}

void Keyboard::Release(char asciiChar) {
  KeyCode keycode = AsciiToKeycode(asciiChar);
#ifdef __APPLE__
  CGEventSourceRef source =
      CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

  CGEventRef event =
      CGEventCreateKeyboardEvent(source, (CGKeyCode)keycode, false);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  CFRelease(source);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(INPUT));
#endif
}

KeyCode Keyboard::AsciiToKeycode(char asciiChar) {
#ifdef __APPLE__
  auto it = asciiToVirtualKeyMap.find(asciiChar);
  if (it == asciiToVirtualKeyMap.end()) {
    std::cerr
        << "Warning: Character not found in the virtual key map. Ignoring..."
        << std::endl;
    return 0xFFFF;  // Return an invalid keycode
  }
  return static_cast<KeyCode>(it->second);
#endif

#ifdef _WIN32
  SHORT vkAndShift = VkKeyScan(asciiChar);
  if (vkAndShift == -1) {
    std::cerr
        << "Warning: Character not found in the virtual key map. Ignoring..."
        << std::endl;
    return 0xFFFF;  // Return an invalid keycode
  }
  return static_cast<KeyCode>(vkAndShift & 0xFF);
#endif
}

std::map<char, int> Keyboard::asciiToVirtualKeyMap = {
    {'0', kVK_ANSI_0},
    {'1', kVK_ANSI_1},
    {'2', kVK_ANSI_2},
    {'3', kVK_ANSI_3},
    {'4', kVK_ANSI_4},
    {'5', kVK_ANSI_5},
    {'6', kVK_ANSI_6},
    {'7', kVK_ANSI_7},
    {'8', kVK_ANSI_8},
    {'9', kVK_ANSI_9},
    {'a', kVK_ANSI_A},
    {'b', kVK_ANSI_B},
    {'c', kVK_ANSI_C},
    {'d', kVK_ANSI_D},
    {'e', kVK_ANSI_E},
    {'f', kVK_ANSI_F},
    {'g', kVK_ANSI_G},
    {'h', kVK_ANSI_H},
    {'i', kVK_ANSI_I},
    {'j', kVK_ANSI_J},
    {'k', kVK_ANSI_K},
    {'l', kVK_ANSI_L},
    {'m', kVK_ANSI_M},
    {'n', kVK_ANSI_N},
    {'o', kVK_ANSI_O},
    {'p', kVK_ANSI_P},
    {'q', kVK_ANSI_Q},
    {'r', kVK_ANSI_R},
    {'s', kVK_ANSI_S},
    {'t', kVK_ANSI_T},
    {'u', kVK_ANSI_U},
    {'v', kVK_ANSI_V},
    {'w', kVK_ANSI_W},
    {'x', kVK_ANSI_X},
    {'y', kVK_ANSI_Y},
    {'z', kVK_ANSI_Z},
    {'(', kVK_ANSI_9},
    {')', kVK_ANSI_0},
    {'`', kVK_ANSI_Grave},
    {'-', kVK_ANSI_Minus},
    {'=', kVK_ANSI_Equal},
    {'\\', kVK_ANSI_Backslash},
    {39, kVK_ANSI_Quote},  // single quote
    {';', kVK_ANSI_Semicolon},
    {',', kVK_ANSI_Comma},
    {'.', kVK_ANSI_Period},
    {'/', kVK_ANSI_Slash},
    {' ', kVK_Space},
};

// TODO: support special keys
//  {"backspace", kVK_Delete},
//  {"delete", kVK_ForwardDelete},
//  {"enter", kVK_Return},
//  {"tab", kVK_Tab},
//  {"escape", kVK_Escape},
//  {"up", kVK_UpArrow},
//  {"down", kVK_DownArrow},
//  {"right", kVK_RightArrow},
//  {"left", kVK_LeftArrow},

}  // namespace Robot

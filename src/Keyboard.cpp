#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

#include <iostream>
#include <random>

#include "./Keyboard.h"
#include "./Utils.h"

namespace Robot {

int Keyboard::delay = 1;

void Keyboard::Type(const std::string &query) {
  for (char c : query) {
    Click(c);
  }
}

void Keyboard::TypeHumanLike(const std::string &query) {
  for (char c : query) {
    Click(c);

    std::normal_distribution<double> distribution(75, 25);
    std::random_device rd;
    std::mt19937 engine(rd());
    Robot::delay((int)distribution(engine));
  }
}

void Keyboard::Click(char asciiChar) {
  Press(asciiChar);
  Release(asciiChar);
}

void Keyboard::Click(SpecialKey specialKey) {
  Press(specialKey);
  Release(specialKey);
}

void Keyboard::Press(char asciiChar) {
  KeyCode keycode = AsciiToVirtualKey(asciiChar);
#ifdef __APPLE__
  CGEventSourceRef source =
      CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
  CGEventRef event = CGEventCreateKeyboardEvent(source, keycode, true);
  CGEventPost(kCGHIDEventTap, event);

  CFRelease(event);
  CFRelease(source);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  SendInput(1, &input, sizeof(INPUT));
#endif

  Robot::delay(delay);
}

void Keyboard::Press(SpecialKey specialKey) {
  KeyCode keycode = SpecialKeyToVirtualKey(specialKey);
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

  Robot::delay(delay);
}

void Keyboard::Release(char asciiChar) {
  KeyCode keycode = AsciiToVirtualKey(asciiChar);
#ifdef __APPLE__
  CGEventRef event =
      CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)keycode, false);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(INPUT));
#endif

  Robot::delay(delay);
}

void Keyboard::Release(SpecialKey specialKey) {
  KeyCode keycode = SpecialKeyToVirtualKey(specialKey);
#ifdef __APPLE__
  CGEventRef event =
      CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)keycode, false);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(INPUT));
#endif

  Robot::delay(delay);
}

KeyCode Keyboard::SpecialKeyToVirtualKey(SpecialKey specialKey) {
  return specialKeyToVirtualKeyMap.at(specialKey);
}

KeyCode Keyboard::AsciiToVirtualKey(char asciiChar) {
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

#ifdef __APPLE__
std::map<Keyboard::SpecialKey, KeyCode> Keyboard::specialKeyToVirtualKeyMap = {
    {Keyboard::BACKSPACE, kVK_Delete},
    {Keyboard::ENTER, kVK_Return},
    {Keyboard::TAB, kVK_Tab},
    {Keyboard::ESCAPE, kVK_Escape},
    {Keyboard::UP, kVK_UpArrow},
    {Keyboard::DOWN, kVK_DownArrow},
    {Keyboard::RIGHT, kVK_RightArrow},
    {Keyboard::LEFT, kVK_LeftArrow},
    {Keyboard::META, kVK_Command},
    {Keyboard::ALT, kVK_Option},
    {Keyboard::CONTROL, kVK_Control},
    {Keyboard::SHIFT, kVK_Shift},
    {Keyboard::CAPSLOCK, kVK_CapsLock}};
#endif

#ifdef _WIN32
std::map<Keyboard::SpecialKey, KeyCode> Keyboard::specialKeyToVirtualKeyMap = {
    {Keyboard::BACKSPACE, VK_BACK},
    {Keyboard::ENTER, VK_RETURN},
    {Keyboard::TAB, VK_TAB},
    {Keyboard::ESCAPE, VK_ESCAPE},
    {Keyboard::UP, VK_UP},
    {Keyboard::DOWN, VK_DOWN},
    {Keyboard::RIGHT, VK_RIGHT},
    {Keyboard::LEFT, VK_LEFT},
    {Keyboard::META, VK_LWIN},
    {Keyboard::ALT, VK_MENU},
    {Keyboard::CONTROL, VK_CONTROL},
    {Keyboard::SHIFT, VK_SHIFT},
    {Keyboard::CAPSLOCK, VK_CAPITAL}};
#endif

#ifdef __APPLE__
std::map<char, int> Keyboard::asciiToVirtualKeyMap = {
    {'0', kVK_ANSI_0},          {'1', kVK_ANSI_1},
    {'2', kVK_ANSI_2},          {'3', kVK_ANSI_3},
    {'4', kVK_ANSI_4},          {'5', kVK_ANSI_5},
    {'6', kVK_ANSI_6},          {'7', kVK_ANSI_7},
    {'8', kVK_ANSI_8},          {'9', kVK_ANSI_9},
    {'a', kVK_ANSI_A},          {'A', kVK_ANSI_A},
    {'b', kVK_ANSI_B},          {'B', kVK_ANSI_B},
    {'c', kVK_ANSI_C},          {'C', kVK_ANSI_C},
    {'d', kVK_ANSI_D},          {'D', kVK_ANSI_D},
    {'e', kVK_ANSI_E},          {'E', kVK_ANSI_E},
    {'f', kVK_ANSI_F},          {'F', kVK_ANSI_F},
    {'g', kVK_ANSI_G},          {'G', kVK_ANSI_G},
    {'h', kVK_ANSI_H},          {'H', kVK_ANSI_H},
    {'i', kVK_ANSI_I},          {'I', kVK_ANSI_I},
    {'j', kVK_ANSI_J},          {'J', kVK_ANSI_J},
    {'k', kVK_ANSI_K},          {'K', kVK_ANSI_K},
    {'l', kVK_ANSI_L},          {'L', kVK_ANSI_L},
    {'m', kVK_ANSI_M},          {'M', kVK_ANSI_M},
    {'n', kVK_ANSI_N},          {'N', kVK_ANSI_N},
    {'o', kVK_ANSI_O},          {'O', kVK_ANSI_O},
    {'p', kVK_ANSI_P},          {'P', kVK_ANSI_P},
    {'q', kVK_ANSI_Q},          {'Q', kVK_ANSI_Q},
    {'r', kVK_ANSI_R},          {'R', kVK_ANSI_R},
    {'s', kVK_ANSI_S},          {'S', kVK_ANSI_S},
    {'t', kVK_ANSI_T},          {'T', kVK_ANSI_T},
    {'u', kVK_ANSI_U},          {'U', kVK_ANSI_U},
    {'v', kVK_ANSI_V},          {'V', kVK_ANSI_V},
    {'w', kVK_ANSI_W},          {'W', kVK_ANSI_W},
    {'x', kVK_ANSI_X},          {'X', kVK_ANSI_X},
    {'y', kVK_ANSI_Y},          {'Y', kVK_ANSI_Y},
    {'z', kVK_ANSI_Z},          {'Z', kVK_ANSI_Z},
    {' ', kVK_Space},           {'(', kVK_ANSI_9},
    {')', kVK_ANSI_0},          {'`', kVK_ANSI_Grave},
    {'-', kVK_ANSI_Minus},      {'=', kVK_ANSI_Equal},
    {'\\', kVK_ANSI_Backslash}, {39, kVK_ANSI_Quote},  // single quote
    {';', kVK_ANSI_Semicolon},  {',', kVK_ANSI_Comma},
    {'.', kVK_ANSI_Period},     {'/', kVK_ANSI_Slash}};

char Keyboard::VirtualKeyToAscii(KeyCode virtualKey) {
  auto map = asciiToVirtualKeyMap;
  auto it = std::find_if(
      map.begin(),
      map.end(),
      [virtualKey](const std::pair<char, int> &p) {
        return p.second == virtualKey;
      }
  );

  if (it == map.end()) {
    std::cerr << "Warning: Virtual key not found in the ascii map. Ignoring..."
              << std::endl;
    return 0xFFFF;  // Return an invalid keycode
  }

  return it->first;
}
#endif
}  // namespace Robot

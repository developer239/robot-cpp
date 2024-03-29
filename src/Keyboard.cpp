#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

#include <iostream>
#include <random>
#include <map>

#include "./Keyboard.h"
#include "./Utils.h"

namespace Robot {

int Keyboard::delay = 1;

const char Keyboard::INVALID_ASCII = static_cast<char>(0xFF);

std::thread Keyboard::keyPressThread;
std::atomic<bool> Keyboard::continueHolding(false);
std::set<char> Keyboard::heldAsciiChars;
std::set<Keyboard::SpecialKey> Keyboard::heldSpecialKeys;

void Keyboard::HoldStart(char asciiChar) {
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = true;
    keyPressThread = std::thread(KeyHoldThread);
  }
  heldAsciiChars.insert(asciiChar);
}

void Keyboard::HoldStart(SpecialKey specialKey) {
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = true;
    keyPressThread = std::thread(KeyHoldThread);
  }
  heldSpecialKeys.insert(specialKey);
}

void Keyboard::HoldStop(char asciiChar) {
  heldAsciiChars.erase(asciiChar);
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = false;
    if (keyPressThread.joinable()) {
      keyPressThread.join();
    }
  }
  Release(asciiChar);
}

void Keyboard::HoldStop(SpecialKey specialKey) {
  heldSpecialKeys.erase(specialKey);
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = false;
    if (keyPressThread.joinable()) {
      keyPressThread.join();
    }
  }
  Release(specialKey);
}

void Keyboard::KeyHoldThread() {
  while (continueHolding) {
    for (char asciiChar : heldAsciiChars) {
      Press(asciiChar);
    }
    for (SpecialKey specialKey : heldSpecialKeys) {
      Press(specialKey);
    }
    Robot::delay(50);
  }

  for (char asciiChar : heldAsciiChars) {
    Release(asciiChar);
  }
  for (SpecialKey specialKey : heldSpecialKeys) {
    Release(specialKey);
  }
}

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
    {Keyboard::CAPSLOCK, kVK_CapsLock},
    {Keyboard::F1, kVK_F1},
    {Keyboard::F2, kVK_F2},
    {Keyboard::F3, kVK_F3},
    {Keyboard::F4, kVK_F4},
    {Keyboard::F5, kVK_F5},
    {Keyboard::F6, kVK_F6},
    {Keyboard::F7, kVK_F7},
    {Keyboard::F8, kVK_F8},
    {Keyboard::F9, kVK_F9},
    {Keyboard::F10, kVK_F10},
    {Keyboard::F11, kVK_F11},
    {Keyboard::F12, kVK_F12}};
#endif

  char Keyboard::VirtualKeyToAscii(KeyCode virtualKey) {
#ifdef __APPLE__
    auto map = asciiToVirtualKeyMap;
  auto it = std::find_if(
      map.begin(),
      map.end(),
      [virtualKey](const std::pair<char, int> &p) {
        return p.second == virtualKey;
      }
  );

  if (it == map.end()) {
    return INVALID_ASCII;
  }

  return it->first;
#endif

#ifdef _WIN32
    // Convert the virtual key code to a scan code
    UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);

    // Convert the scan code to the corresponding character
    char character = 0;
    BYTE keyboardState[256] = {0};
    GetKeyboardState(keyboardState);
    wchar_t buffer[2];
    if (ToUnicode(virtualKey, scanCode, keyboardState, buffer, 2, 0) == 1) {
      character = static_cast<char>(buffer[0]);
    }
    return character;
#endif
  }

  Keyboard::SpecialKey Keyboard::VirtualKeyToSpecialKey(KeyCode virtualKey) {
#ifdef __APPLE__
    switch (virtualKey) {
    case 123:
      return Keyboard::LEFT;
    case 124:
      return Keyboard::RIGHT;
    case 125:
      return Keyboard::DOWN;
    case 126:
      return Keyboard::UP;
    case 36:
      return Keyboard::ENTER;
    case 48:
      return Keyboard::TAB;
    case 51:
      return Keyboard::BACKSPACE;
    case 53:
      return Keyboard::ESCAPE;
    case 55:
      return Keyboard::META;
    case 56:
      return Keyboard::SHIFT;
    case 57:
      return Keyboard::CAPSLOCK;
    case 58:
      return Keyboard::ALT;
    case 59:
      return Keyboard::CONTROL;
  }
#endif

#ifdef _WIN32
    switch (virtualKey) {
      case VK_LEFT:
        return Keyboard::LEFT;
      case VK_RIGHT:
        return Keyboard::RIGHT;
      case VK_DOWN:
        return Keyboard::DOWN;
      case VK_UP:
        return Keyboard::UP;
      case VK_RETURN:
        return Keyboard::ENTER;
      case VK_TAB:
        return Keyboard::TAB;
      case VK_BACK:
        return Keyboard::BACKSPACE;
      case VK_ESCAPE:
        return Keyboard::ESCAPE;
      case VK_LWIN:
      case VK_RWIN:
        return Keyboard::META;
      case VK_SHIFT:
        return Keyboard::SHIFT;
      case VK_CAPITAL:
        return Keyboard::CAPSLOCK;
      case VK_MENU:
        return Keyboard::ALT;
      case VK_CONTROL:
        return Keyboard::CONTROL;
    }
#endif
  }

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
    {Keyboard::CAPSLOCK, VK_CAPITAL},
    {Keyboard::F1, VK_F1},
    {Keyboard::F2, VK_F2},
    {Keyboard::F3, VK_F3},
    {Keyboard::F4, VK_F4},
    {Keyboard::F5, VK_F5},
    {Keyboard::F6, VK_F6},
    {Keyboard::F7, VK_F7},
    {Keyboard::F8, VK_F8},
    {Keyboard::F9, VK_F9},
    {Keyboard::F10, VK_F10},
    {Keyboard::F11, VK_F11},
    {Keyboard::F12, VK_F12}};
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
#endif
}  // namespace Robot

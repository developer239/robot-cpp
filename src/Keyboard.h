#pragma once

#include <cctype>
#include <string>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif

#include <atomic>
#include <chrono>
#include <set>
#include <thread>

namespace Robot {

#ifdef __APPLE__
typedef CGKeyCode KeyCode;
#endif

#ifdef _WIN32
typedef WORD KeyCode;
#endif

class Keyboard {
 public:
  enum SpecialKey {
    BACKSPACE,
    ENTER,
    TAB,
    ESCAPE,
    UP,
    DOWN,
    RIGHT,
    LEFT,
    META,
    ALT,
    CONTROL,
    SHIFT,
    CAPSLOCK
  };

  static const char INVALID_ASCII;

  Keyboard() = delete;
  virtual ~Keyboard() = default;

  static void Type(const std::string& query);

  static void TypeHumanLike(const std::string& query);

  static void Click(char asciiChar);
  static void Click(SpecialKey specialKey);

  static void HoldStart(char asciiChar);
  static void HoldStart(SpecialKey specialKey);
  static void HoldStop(char asciiChar);
  static void HoldStop(SpecialKey specialKey);

  static void Press(char asciiChar);
  static void Press(SpecialKey specialKey);

  static void Release(char asciiChar);
  static void Release(SpecialKey specialKey);

  static char VirtualKeyToAscii(KeyCode virtualKey);
  static SpecialKey VirtualKeyToSpecialKey(KeyCode virtualKey);

 private:
  static std::thread keyPressThread;
  static std::atomic<bool> continueHolding;
  static std::set<char> heldAsciiChars;
  static std::set<SpecialKey> heldSpecialKeys;

  static void KeyHoldThread();

  static int delay;

  static KeyCode AsciiToVirtualKey(char asciiChar);

  static KeyCode SpecialKeyToVirtualKey(SpecialKey specialKey);

  static std::map<SpecialKey, KeyCode> specialKeyToVirtualKeyMap;

  // note: windows alternative doesn't use map
#ifdef __APPLE__
  static std::map<char, int> asciiToVirtualKeyMap;
#endif
};

}  // namespace Robot

#pragma once

#include <cctype>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#include <map>
#endif

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

  Keyboard() = delete;
  virtual ~Keyboard() = default;

  static void Type(const std::string& query);

  static void TypeHumanLike(const std::string& query);

  static void Click(char asciiChar);
  static void Click(SpecialKey specialKey);

  static void Press(char asciiChar);
  static void Press(SpecialKey specialKey);

  static void Release(char asciiChar);
  static void Release(SpecialKey specialKey);

 private:
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

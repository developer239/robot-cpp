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
  Keyboard() = delete;
  virtual ~Keyboard() = default;

  static void Type(const std::string& query);

  static void Click(char asciiChar);

  static void Press(char asciiChar);

  static void Release(char asciiChar);

 private:
  static KeyCode AsciiToKeycode(char asciiChar);

#ifdef __APPLE__
  static std::map<char, int> asciiToVirtualKeyMap;
#endif
};

}  // namespace Robot

#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <optional>

// macOS internal: which key of the current keyboard layout types a character.
// Text goes out on that key so its keydown and keyup name the same key: an
// application that tracks held keys by character (Figma's hold-Z zoom, space to
// pan) otherwise sees a key go down and a different one come up, and treats the
// first as held from then on.
namespace robot::mac {

struct KeyStroke {
  CGKeyCode keycode;
  CGEventFlags flags;
};

// F20 types no character, so a keyup an application derives from this keycode
// names no character key. It carries the characters no key of the layout types.
inline constexpr CGKeyCode kTextOnlyKeycode = 0x5A;

// The key and modifier flags (none, Shift, Option, or both) that type the
// character on the current keyboard layout in one keystroke. nullopt when no
// single keystroke types it: an emoji, a letter the layout lacks, or one the
// layout reaches only through a dead key. The layout is read through Text Input
// Sources on the calling thread; robot-ts calls on the process's main thread.
std::optional<KeyStroke> keyStrokeForCharacter(char32_t codepoint);

}  // namespace robot::mac

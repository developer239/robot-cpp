#pragma once

#include <Windows.h>

#include "robot/Key.h"

// Windows internal: map the library's position-based Key (USB HID usage) to the
// hardware scan code used for SendInput with KEYEVENTF_SCANCODE, and translate a
// hook's scan code back to a physical Key for recording.
//
// Scan codes, not virtual keys, are the correct primitive for positional
// injection: a virtual key is layout-dependent (VK_A is wherever the current
// layout puts 'a'), whereas a scan code names the physical key, so Key::A always
// drives the top-left letter key regardless of layout. Extended keys (arrows,
// navigation cluster, right-hand modifiers, keypad Enter/Divide) require the
// KEYEVENTF_EXTENDEDKEY flag alongside their scan code; needsExtended reports
// that.
namespace robot::win {

struct ScanCode {
  WORD code = 0;       // Set 1 hardware scan code.
  bool extended = false;  // Requires KEYEVENTF_EXTENDEDKEY.
  bool valid = false;
};

// Physical Key -> scan code. valid == false for keys with no PC scan code, which
// the keyboard backend reports as ErrorCode::UnmappableInput.
ScanCode keyToScanCode(Key key);

// Hook scan code (+ extended flag) -> physical Key for recording. Key::Unknown
// for codes the library does not model.
Key scanCodeToKey(WORD scanCode, bool extended);

}  // namespace robot::win
#pragma once

#include <X11/Xlib.h>

#include "robot/Key.h"

// Linux/X11 internal: map the library's position-based Key (USB HID usage) to an
// X11 keysym, and translate a keysym back to a physical Key for recording.
//
// Two layers of translation exist on X11 and must not be confused. A keysym is a
// layout-level symbol (XK_a, XK_Left); the server maps keysyms to keycodes
// through the active layout. For positional injection the backend looks up the
// keysym for a Key here, then asks the server for the keycode currently bound to
// that keysym (XKeysymToKeycode). That keeps Key::A on the physical A-position
// key for the common Latin layouts while still going through the server's own
// mapping. Arbitrary Unicode text does NOT go through this table at all - it uses
// a temporary keysym remap in the mouse/keyboard backend (see typeUnicode).
namespace robot::x11 {

// Physical Key -> X11 keysym. NoSymbol for keys with no sensible X keysym.
KeySym keyToKeysym(Key key);

// X11 keysym -> physical Key for recording. Key::Unknown when not modeled.
Key keysymToKey(KeySym keysym);

}  // namespace robot::x11
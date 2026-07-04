#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <optional>

#include "robot/Key.h"

// macOS internal: translation between the library's position-based Key (USB HID
// usage) and macOS virtual key codes (CGKeyCode / the kVK_* numbering), plus
// modifier-flag helpers. macOS virtual key codes are NOT HID usages - they are
// Apple's own numbering - so a real mapping table is required; the old code's
// assumption that a character map was sufficient is what made non-US layouts and
// capitals wrong.
namespace robot::mac {

// Physical Key -> CGKeyCode. nullopt for keys with no macOS virtual-key
// equivalent (PrintScreen, ScrollLock, Pause, Insert, Windows Menu key, ...),
// which the keyboard backend reports as ErrorCode::UnmappableInput rather than
// pressing something arbitrary.
std::optional<CGKeyCode> keyToMacKeycode(Key key);

// CGKeyCode -> physical Key for the event tap (recording). Key::Unknown for
// codes the library does not model.
Key macKeycodeToKey(CGKeyCode keycode);

// The CGEvent flag bit for a modifier key, or 0 if the key is not a modifier.
// The keyboard backend ORs these over all currently-held modifier keys and sets
// the result on every synthesized event, which is how Command+C reaches other
// applications correctly (a CGEvent carries its modifiers as flags, not as
// separately-posted key events).
CGEventFlags macModifierFlag(Key key);

bool isModifierKey(Key key);

}  // namespace robot::mac
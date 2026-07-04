#pragma once

#include <cstdint>
#include <optional>

#include "robot/Key.h"

// Linux/uinput internal: physical Key -> evdev KEY_* code. evdev keycodes are the
// kernel's own numbering (Linux input event codes), distinct from both HID usages
// and X keysyms, so a dedicated table is required. nullopt for keys with no
// evdev equivalent.
namespace robot::linux_evdev {

std::optional<std::uint16_t> keyToEvdev(Key key);

}  // namespace robot::linux_evdev
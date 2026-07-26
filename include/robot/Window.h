#pragma once

#include <cstdint>
#include <string>

#include "robot/Geometry.h"

namespace robot {

// One on-screen application window, as reported by enumeration. Windows move
// and resize after enumeration - query Windows::frame() for the current
// bounds before doing coordinate math against one.
struct Window {
  std::uint32_t id = 0;
  std::int32_t ownerPid = 0;
  std::string appName;
  std::string title;
  // Global logical desktop coordinates (top-left origin), at enumeration time.
  LogicalRect frame;
};

}  // namespace robot

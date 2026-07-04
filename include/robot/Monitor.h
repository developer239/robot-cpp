#pragma once

#include <cstdint>
#include <string>

#include "robot/Geometry.h"

namespace robot {

// One physical display in the virtual desktop. Both coordinate spaces are
// carried explicitly so callers never have to guess a display's density:
//
//   logicalBounds  : origin and size in global logical units. Cursor moves and
//                    window-space math operate here. Origins can be negative
//                    (a monitor left of or above the primary) and non-primary
//                    monitors are placed relative to the primary's origin.
//   physicalBounds : the same region in device pixels; screen capture uses this.
//   scaleFactor    : physical pixels per logical unit for this display (2.0 on a
//                    typical Retina panel, 1.5 at 150% on Windows). Different
//                    monitors can have different factors in one session, which is
//                    exactly why a single global scale is not enough.
//
// Two displays never share an id within a session, but ids are not stable across
// sessions or hot-plug events; re-enumerate rather than caching them.
struct Monitor {
  std::uint32_t id = 0;
  std::string name;
  bool isPrimary = false;
  LogicalRect logicalBounds;
  PhysicalRect physicalBounds;
  double scaleFactor = 1.0;
};

}  // namespace robot
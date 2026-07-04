#pragma once

#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace robot {

// Whether a scroll delta is expressed in wheel notches (lines) or in device
// pixels. Pixel deltas drive high-resolution / precision scrolling; a backend
// reports support via Capabilities::supportsHighResolutionScroll and fails with
// ErrorCode::Unsupported rather than silently rounding pixels to lines.
enum class ScrollUnit : std::uint8_t { Line, Pixel };

constexpr std::string_view toString(const ScrollUnit unit) {
  switch (unit) {
    case ScrollUnit::Line: return "Line";
    case ScrollUnit::Pixel: return "Pixel";
  }
  std::abort();
}

// A scroll amount along both axes.
//
// Sign convention, applied before any OS "natural scrolling" setting:
//   vertical   > 0 scrolls up   (wheel rolled away from the user)
//   vertical   < 0 scrolls down
//   horizontal > 0 scrolls right
//   horizontal < 0 scrolls left
//
// A backend maps these to its native direction; the OS may then invert them if
// natural scrolling is enabled. That inversion is a user setting, not a library
// behaviour, so it is not hidden here.
struct ScrollDelta {
  double horizontal = 0.0;
  double vertical = 0.0;
  ScrollUnit unit = ScrollUnit::Line;

  static ScrollDelta lines(
      const double vertical, const double horizontal = 0.0
  ) {
    return {.horizontal = horizontal,
            .vertical = vertical,
            .unit = ScrollUnit::Line};
  }
  static ScrollDelta pixels(
      const double vertical, const double horizontal = 0.0
  ) {
    return {.horizontal = horizontal,
            .vertical = vertical,
            .unit = ScrollUnit::Pixel};
  }
};

}  // namespace robot

#pragma once

#include <chrono>
#include <expected>
#include <vector>

#include "robot/Error.h"
#include "robot/Event.h"

namespace robot {

class Session;

// Options controlling replay pacing.
struct ReplayOptions {
  // Multiplies every inter-event gap: 2.0 replays at half speed, 0.5 at double.
  double timeScale = 1.0;
  // Upper bound on any single wait, so a long idle gap in the recording does not
  // freeze replay. Zero disables the cap.
  std::chrono::milliseconds maxGap{0};
};

// Captures a timeline of normalized input events and replays it through a
// Session. Replacing the previous design's runtime-polymorphic Action hierarchy:
// events are a std::variant stored contiguously (no per-event heap allocation,
// no dynamic_cast in the replay loop), dispatched by std::visit. Timing is
// driven off a steady clock against timestamps relative to recording start, so a
// timeline replays with the same cadence every time.
class Recorder {
 public:
  Recorder() = default;

  // Append an event, stamped with the elapsed time since the first recorded
  // event (or since reset). Typically wired to an EventTap as its sink.
  void capture(const InputEvent& event);

  [[nodiscard]] const std::vector<RecordedEvent>& events() const {
    return events_;
  }
  [[nodiscard]] bool empty() const { return events_.empty(); }

  // Discard all events and restart the clock at the next capture.
  void reset();

  // Replay every event in order against the given Session, honoring the recorded
  // gaps (scaled/capped per options). Physical key events replay by position and
  // are therefore layout-independent. Returns the first error encountered.
  [[nodiscard]] std::expected<void, Error> replay(
      Session& session, const ReplayOptions& options = {}
  ) const;

 private:
  std::vector<RecordedEvent> events_;
  bool started_ = false;
  std::chrono::steady_clock::time_point start_{};
};

}  // namespace robot
#include "robot/Recorder.h"

#include <chrono>
#include <cmath>
#include <thread>
#include <variant>

#include "robot/Session.h"

namespace robot {
namespace {

template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};

}  // namespace

void Recorder::capture(const InputEvent& event) {
  const auto now = std::chrono::steady_clock::now();
  if (!started_) {
    started_ = true;
    start_ = now;
  }
  const auto timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
  events_.push_back({.timestamp = timestamp, .event = event});
}

void Recorder::reset() {
  events_.clear();
  started_ = false;
}

std::expected<void, Error> Recorder::replay(
    Session& session, const ReplayOptions& options
) const {
  if (events_.empty()) return {};

  Keyboard& keyboard = session.keyboard();
  Mouse& mouse = session.mouse();

  // Dispatch a normalized event to the injection facades. Button and scroll
  // events warp to their recorded position first so replay reproduces location.
  const auto dispatch = [&](const InputEvent& e) -> std::expected<void, Error> {
    return std::visit(
        Overloaded{
            [&](const KeyEvent& k) -> std::expected<void, Error> {
              return k.down ? keyboard.press(k.key) : keyboard.release(k.key);
            },
            [&](const MouseMoveEvent& m) -> std::expected<void, Error> {
              return mouse.move(m.position);
            },
            [&](const MouseButtonEvent& m) -> std::expected<void, Error> {
              if (auto r = mouse.move(m.position); !r) return r;
              return m.down ? mouse.press(m.button) : mouse.release(m.button);
            },
            [&](const ScrollEvent& s) -> std::expected<void, Error> {
              if (auto r = mouse.move(s.position); !r) return r;
              return mouse.scroll(s.delta);
            },
        },
        e
    );
  };

  // Timing is reconstructed from per-event gaps (not absolute timestamps) so
  // timeScale and maxGap apply cleanly and a long idle gap cannot freeze replay.
  const auto replayStart = std::chrono::steady_clock::now();
  std::chrono::milliseconds virtualElapsed{0};
  std::chrono::milliseconds previous = events_.front().timestamp;

  for (const RecordedEvent& rec : events_) {
    std::chrono::milliseconds gap = rec.timestamp - previous;
    previous = rec.timestamp;

    auto scaled = std::chrono::milliseconds(
        static_cast<long long>(std::llround(gap.count() * options.timeScale))
    );
    if (options.maxGap.count() > 0 && scaled > options.maxGap) {
      scaled = options.maxGap;
    }
    virtualElapsed += scaled;

    std::this_thread::sleep_until(replayStart + virtualElapsed);

    if (auto r = dispatch(rec.event); !r) {
      return std::unexpected(r.error());
    }
  }
  return {};
}

}  // namespace robot
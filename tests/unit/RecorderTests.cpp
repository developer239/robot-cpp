#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "robot/Recorder.h"

// The recorder timeline is pure data: events stamped with elapsed time, stored
// contiguously in a variant. These tests verify capture ordering and timestamp
// monotonicity without any OS involvement (replay itself needs a Session and is
// covered by the interactive suite / a Session built on the mock).
namespace robot {
namespace {

TEST(Recorder, StampsEventsRelativeToFirstCapture) {
  Recorder recorder;
  EXPECT_TRUE(recorder.empty());

  recorder.capture(KeyEvent{Key::A, true});
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  recorder.capture(KeyEvent{Key::A, false});

  const auto& events = recorder.events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events.front().timestamp.count(), 0);
  EXPECT_GE(events.back().timestamp.count(), events.front().timestamp.count());
  EXPECT_TRUE(std::holds_alternative<KeyEvent>(events.front().event));
}

TEST(Recorder, ResetClearsAndRestartsClock) {
  Recorder recorder;
  recorder.capture(MouseMoveEvent{{10.0, 20.0}});
  recorder.reset();
  EXPECT_TRUE(recorder.empty());

  recorder.capture(MouseButtonEvent{MouseButton::Left, true, {5.0, 5.0}});
  ASSERT_EQ(recorder.events().size(), 1u);
  EXPECT_EQ(recorder.events().front().timestamp.count(), 0);
}

}  // namespace
}  // namespace robot
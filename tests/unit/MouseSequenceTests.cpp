#include <gtest/gtest.h>

#include "robot/Mouse.h"
#include "support/MockBackend.h"

// The high-level gestures are portable compositions of atomic backend calls.
// Asserting the recorded call sequence pins their behaviour identically on every
// platform, and catches regressions like a double-click that doesn't report a
// second-click count or a drag that releases before moving.
namespace robot::test {
namespace {

TEST(MouseSequence, DoubleClickReportsSecondClickCount) {
  MockPlatformBackend backend;
  Mouse mouse(backend.mouse());

  ASSERT_TRUE(mouse.doubleClick(MouseButton::Left).has_value());

  const auto& log = backend.log();
  ASSERT_EQ(log.size(), 4u);
  EXPECT_EQ(log[0].action, ButtonAction::Down);
  EXPECT_EQ(log[0].clickCount, 1);
  EXPECT_EQ(log[2].action, ButtonAction::Down);
  EXPECT_EQ(log[2].clickCount, 2);  // Native double-click detection depends on this.
}

TEST(MouseSequence, DragPressesMovesThenReleases) {
  MockPlatformBackend backend;
  backend.mockMouse().setPosition({0.0, 0.0});
  Mouse mouse(backend.mouse());

  ASSERT_TRUE(mouse.drag({100.0, 50.0}, MouseButton::Left).has_value());

  const auto& log = backend.log();
  // Order must be: button down, warp to target, button up.
  ASSERT_GE(log.size(), 3u);
  EXPECT_EQ(log.front().kind, RecordedCall::Kind::Button);
  EXPECT_EQ(log.front().action, ButtonAction::Down);
  EXPECT_EQ(log.back().kind, RecordedCall::Kind::Button);
  EXPECT_EQ(log.back().action, ButtonAction::Up);

  bool warped = false;
  for (const auto& c : log) {
    if (c.kind == RecordedCall::Kind::Warp && c.point == LogicalPoint{100.0, 50.0}) {
      warped = true;
    }
  }
  EXPECT_TRUE(warped);
}

TEST(MouseSequence, SmoothMoveEndsAtTarget) {
  MockPlatformBackend backend;
  backend.mockMouse().setPosition({0.0, 0.0});
  Mouse mouse(backend.mouse());

  MouseMoveOptions options;
  options.duration = std::chrono::milliseconds(0);  // No sleeping in the test.
  options.steps = 10;
  ASSERT_TRUE(mouse.moveSmooth({100.0, 0.0}, options).has_value());

  // Last warp must land exactly on the target regardless of interpolation.
  LogicalPoint last;
  for (const auto& c : backend.log()) {
    if (c.kind == RecordedCall::Kind::Warp) last = c.point;
  }
  EXPECT_EQ(last, (LogicalPoint{100.0, 0.0}));
}

}  // namespace
}  // namespace robot::test

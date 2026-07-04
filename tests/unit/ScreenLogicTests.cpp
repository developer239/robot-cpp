#include <gtest/gtest.h>

#include "robot/Screen.h"
#include "support/MockBackend.h"

// The Screen facade derives primary-first ordering, virtual-desktop bounds, and
// per-monitor capture from just enumerateMonitors + captureRegion. Feeding the
// mock a mixed-DPI, negative-origin monitor layout verifies that derived math
// once, portably, instead of per-OS.
namespace robot::test {
namespace {

std::vector<Monitor> twoMonitorLayout() {
  Monitor left;
  left.id = 2;
  left.isPrimary = false;
  left.scaleFactor = 1.0;
  left.physicalBounds = PhysicalRect{{-1920, 0}, {1920, 1080}};
  left.logicalBounds = LogicalRect{{-1920.0, 0.0}, {1920.0, 1080.0}};

  Monitor primary;
  primary.id = 1;
  primary.isPrimary = true;
  primary.scaleFactor = 2.0;  // Retina-like.
  primary.physicalBounds = PhysicalRect{{0, 0}, {5120, 2880}};
  primary.logicalBounds = LogicalRect{{0.0, 0.0}, {2560.0, 1440.0}};

  return {left, primary};  // Deliberately primary-second to test reordering.
}

TEST(ScreenLogic, PutsPrimaryFirst) {
  MockPlatformBackend backend;
  backend.mockScreen().setMonitors(twoMonitorLayout());
  Screen screen(backend.screen());

  auto monitors = screen.monitors();
  ASSERT_TRUE(monitors.has_value());
  ASSERT_EQ(monitors->size(), 2u);
  EXPECT_TRUE(monitors->front().isPrimary);
  EXPECT_EQ(monitors->front().id, 1u);
}

TEST(ScreenLogic, ComputesVirtualBoundsAcrossNegativeOrigin) {
  MockPlatformBackend backend;
  backend.mockScreen().setMonitors(twoMonitorLayout());
  Screen screen(backend.screen());

  auto bounds = screen.virtualBounds();
  ASSERT_TRUE(bounds.has_value());
  // Union spans x from -1920 to 5120 and y from 0 to 2880.
  EXPECT_EQ(bounds->origin.x, -1920);
  EXPECT_EQ(bounds->origin.y, 0);
  EXPECT_EQ(bounds->size.width, 1920 + 5120);
  EXPECT_EQ(bounds->size.height, 2880);
}

TEST(ScreenLogic, CaptureMonitorUsesPhysicalBounds) {
  MockPlatformBackend backend;
  backend.mockScreen().setMonitors(twoMonitorLayout());
  Screen screen(backend.screen());

  auto image = screen.captureMonitor(1);
  ASSERT_TRUE(image.has_value());
  // The primary is 5120x2880 physical pixels; the facade must request exactly
  // that, proving it captures at native resolution rather than logical size.
  EXPECT_EQ(image->width(), 5120);
  EXPECT_EQ(image->height(), 2880);
}

TEST(ScreenLogic, UnknownMonitorFails) {
  MockPlatformBackend backend;
  backend.mockScreen().setMonitors(twoMonitorLayout());
  Screen screen(backend.screen());

  auto image = screen.captureMonitor(999);
  ASSERT_FALSE(image.has_value());
  EXPECT_EQ(image.error().code, ErrorCode::MonitorNotFound);
}

}  // namespace
}  // namespace robot::test
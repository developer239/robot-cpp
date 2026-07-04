#include <gtest/gtest.h>

#include "robot/Geometry.h"

namespace robot {
namespace {

TEST(Geometry, ConvertsLogicalPointToPhysicalWithScale) {
  EXPECT_EQ(toPhysical(LogicalPoint{10.25, 20.5}, 2.0), (PhysicalPoint{21, 41}));
}

TEST(Geometry, ConvertsPhysicalPointToLogicalWithScale) {
  EXPECT_EQ(toLogical(PhysicalPoint{21, 41}, 2.0), (LogicalPoint{10.5, 20.5}));
}

TEST(Geometry, RectContainsUsesHalfOpenBounds) {
  const PhysicalRect rect{{10, 20}, {30, 40}};

  EXPECT_TRUE(rect.contains({10, 20}));
  EXPECT_TRUE(rect.contains({39, 59}));
  EXPECT_FALSE(rect.contains({40, 60}));
}

}  // namespace
}  // namespace robot

#include <gtest/gtest.h>

#include "./Mouse.h"
#include "./Utils.h"

TEST(MouseTest, MouseMove) {
  Robot::delay(2000);
  Robot::Point newPos{100, 100};
  Robot::Mouse::Move(newPos);

  Robot::Point currentPos = Robot::Mouse::GetPosition();

  EXPECT_EQ(currentPos.x, 100);
  EXPECT_EQ(currentPos.y, 100);
}

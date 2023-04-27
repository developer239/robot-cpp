#include <gtest/gtest.h>

#include "./Keyboard.h"
#include "./Mouse.h"
#include "./Utils.h"

TEST(MouseTest, MouseMove) {
  Robot::Point newPos{100, 100};
  Robot::Mouse::Move(newPos);

  Robot::Point currentPos = Robot::Mouse::GetPosition();

  EXPECT_EQ(currentPos.x, 100);
  EXPECT_EQ(currentPos.y, 100);
}

TEST(KeyboardTest, KeyboardType) {
  Robot::Keyboard::Press(Robot::Keyboard::SpecialKey::SHIFT);
  std::string query = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
  Robot::Keyboard::TypeHumanLike(query);
  Robot::Keyboard::Release(Robot::Keyboard::SpecialKey::SHIFT);
  Robot::Keyboard::TypeHumanLike(query);
  Robot::delay(2000);
}

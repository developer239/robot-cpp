//#include <gtest/gtest.h>
//#include <gmock/gmock.h>
//
//#include "Mouse.h"
//
//using namespace Robot;
//using namespace testing;
//
//class MouseTest : public Test {
// protected:
//  void SetUp() override {
//    // Code that runs before each test case
//  }
//
//  void TearDown() override {
//    // Code that runs after each test case
//  }
//};
//
//// Test case to ensure that mouse movement works as expected
//TEST_F(MouseTest, MouseMove) {
//  // Get the initial position of the mouse
//  Point initialPos = Mouse::getMousePos();
//
//  // Move the mouse to a new position
//  Point newPos{initialPos.x + 50, initialPos.y + 50};
//  Mouse::moveMouse(newPos);
//
//  // Verify that the mouse has been moved to the new position
//  Point currentPos = Mouse::getMousePos();
//  ASSERT_EQ(newPos.x, currentPos.x);
//  ASSERT_EQ(newPos.y, currentPos.y);
//}
//
//// Add more test cases for other methods in the Mouse class

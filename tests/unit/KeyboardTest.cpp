#include <gtest/gtest.h>
#include "../../src/Keyboard.h"

// These tests focus on public API validation, not actual keyboard input

TEST(KeyboardTest, SpecialKeyConstants) {
    // Verify that special keys are distinct
    EXPECT_NE(Robot::Keyboard::ENTER, Robot::Keyboard::ESCAPE);
    EXPECT_NE(Robot::Keyboard::TAB, Robot::Keyboard::BACKSPACE);
    EXPECT_NE(Robot::Keyboard::UP, Robot::Keyboard::DOWN);
    EXPECT_NE(Robot::Keyboard::LEFT, Robot::Keyboard::RIGHT);
}

TEST(KeyboardTest, InvalidAsciiConstant) {
    // Check that the invalid ASCII constant is correctly defined
    EXPECT_EQ(Robot::Keyboard::INVALID_ASCII, static_cast<char>(0xFF));
}

TEST(KeyboardTest, VirtualKeyToAscii) {
    // This is a public method we can test
    // We can't test specific key codes due to platform differences,
    // but we can validate general behavior

    // Virtual key 0xFFFF should return INVALID_ASCII
    char result = Robot::Keyboard::VirtualKeyToAscii(0xFFFF);
    EXPECT_EQ(result, Robot::Keyboard::INVALID_ASCII);
}

// Note: Testing actual keyboard input would require the SDL test app

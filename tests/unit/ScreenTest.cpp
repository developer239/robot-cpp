#include <gtest/gtest.h>
#include "../../src/Screen.h"

// Basic tests for Screen functionality
// These tests focus on API behavior, not actual screen captures

TEST(ScreenTest, GetScreenSizeReturnsPositiveValues) {
    Robot::Screen screen;
    Robot::DisplaySize size = screen.GetScreenSize();

    EXPECT_GT(size.width, 0);
    EXPECT_GT(size.height, 0);
}

TEST(ScreenTest, CaptureWithDefaultParametersWorks) {
    Robot::Screen screen;

    // Just verify that this doesn't crash
    EXPECT_NO_THROW(screen.Capture());

    // After capture, pixels should exist
    auto pixels = screen.GetPixels();
    EXPECT_FALSE(pixels.empty());
}

TEST(ScreenTest, PixelOutOfBoundsReturnsBlack) {
    Robot::Screen screen;
    screen.Capture(0, 0, 100, 100);

    // Getting pixels outside the capture area should return black
    Robot::Pixel pixelOutside = screen.GetPixelColor(1000, 1000);

    EXPECT_EQ(pixelOutside.r, 0);
    EXPECT_EQ(pixelOutside.g, 0);
    EXPECT_EQ(pixelOutside.b, 0);
}

// Note: Testing actual screen capture and color accuracy would require the SDL test app

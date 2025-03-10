#include <gtest/gtest.h>
#include "../../src/Mouse.h"
#include "../../src/types.h"

// These tests are focused on computation and utilities, not actual mouse movement

TEST(MouseTest, PointDistanceCalculation) {
    Robot::Point p1{0, 0};
    Robot::Point p2{3, 4};

    // Should be Pythagorean distance of 5
    EXPECT_EQ(p1.Distance(p2), 5.0);
    EXPECT_EQ(p2.Distance(p1), 5.0); // Should be symmetric
}

TEST(MouseTest, SamePointDistanceIsZero) {
    Robot::Point p{100, 200};

    EXPECT_EQ(p.Distance(p), 0.0);
}

// Add more tests for computational aspects
// Note: Testing the actual mouse movement would require the SDL test app

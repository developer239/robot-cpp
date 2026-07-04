#include <gtest/gtest.h>

#include "robot/Key.h"

namespace robot {
namespace {

TEST(Key, UsesHidUsageValues) {
  EXPECT_EQ(keyToHidUsage(Key::A), 0x04);
  EXPECT_EQ(keyToHidUsage(Key::LeftControl), 0xE0);
}

TEST(Key, FormatsKnownKeys) {
  EXPECT_EQ(toString(Key::A), "A");
  EXPECT_EQ(toString(Key::RightMeta), "RightMeta");
}

}  // namespace
}  // namespace robot

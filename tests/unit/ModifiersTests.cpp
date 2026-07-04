#include <gtest/gtest.h>

#include "robot/Modifiers.h"

namespace robot {
namespace {

TEST(Modifiers, StoresAndRemovesFlags) {
  const Modifiers modifiers = Modifier::Control | Modifier::Shift;

  EXPECT_TRUE(modifiers.has(Modifier::Control));
  EXPECT_TRUE(modifiers.has(Modifier::Shift));
  EXPECT_FALSE(modifiers.has(Modifier::Alt));
  EXPECT_FALSE(modifiers.without(Modifier::Shift).has(Modifier::Shift));
}

TEST(Modifiers, FormatsInStableOrder) {
  const Modifiers modifiers = Modifier::Shift | Modifier::Control | Modifier::Meta;

  EXPECT_EQ(toString(modifiers), "Control+Shift+Meta");
  EXPECT_EQ(toString(Modifiers{}), "");
}

}  // namespace
}  // namespace robot

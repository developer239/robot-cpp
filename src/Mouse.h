#pragma once

#include "./types.h"

#include <cstddef>
#include <cstdint>

namespace Robot {

enum class MouseButton : uint8_t {
  LEFT_BUTTON = 0,
  RIGHT_BUTTON = 1,
  CENTER_BUTTON = 2
};

class Mouse {
 public:
  Mouse() = delete;

  static void moveMouse(Robot::Point point);

  static void dragMouse(Robot::Point point, const MouseButton button);

  static bool smoothlyMoveMouse(Robot::Point point, double speed);

  static Robot::Point getMousePos();

  static void toggleMouse(bool down, MouseButton button);

  static void clickMouse(MouseButton button);

  static void doubleClick(MouseButton button);

  static void scrollMouse(int x, int y);
};

}  // namespace Robot

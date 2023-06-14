#pragma once

#include "./types.h"

#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace Robot {

enum class MouseButton : uint8_t {
  LEFT_BUTTON = 0,
  RIGHT_BUTTON = 1,
  CENTER_BUTTON = 2
};

class Mouse {
 public:
  static unsigned int delay;

  static bool isPressed;
  static MouseButton pressedButton;

  Mouse() = delete;

  static void Move(Robot::Point point);

  static void MoveSmooth(Robot::Point point);

  static void Drag(Robot::Point toPoint);

  static void DragSmooth(Robot::Point toPoint);

  static Robot::Point GetPosition();

  static void ToggleButton(
      bool down, MouseButton button, bool doubleClick = false
  );

  static void Click(MouseButton button);

  static void DoubleClick(MouseButton button);

  static void ScrollBy(int y, int x = 0);

 private:
  static void MoveWithButtonPressed(Robot::Point point, MouseButton button);

#ifdef _WIN32
  static POINT getCurrentPosition();
#elif __APPLE__
  static CGPoint getCurrentPosition();
#endif
};

}  // namespace Robot

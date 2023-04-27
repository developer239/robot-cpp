#include "./Mouse.h"
#include "./Utils.h"

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace Robot {

unsigned int Mouse::delay = 16;

#ifdef _WIN32
POINT Mouse::getCurrentPosition() {
  POINT winPoint;
  GetCursorPos(&winPoint);
  return winPoint;
}
#elif __APPLE__
CGPoint Mouse::getCurrentPosition() {
  CGEventRef event = CGEventCreate(nullptr);
  CGPoint cursor = CGEventGetLocation(event);
  CFRelease(event);
  return cursor;
}
#endif

void Mouse::Move(Robot::Point point) {
#ifdef _WIN32
  SetCursorPos(point.x, point.y);
#elif __APPLE__
  CGPoint target = CGPointMake(point.x, point.y);
  CGEventRef event = CGEventCreateMouseEvent(
      nullptr,
      kCGEventMouseMoved,
      target,
      kCGMouseButtonLeft
  );
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif
}

void Mouse::MoveSmooth(Robot::Point point, double speed) {
  if (speed <= 0) {
    throw std::invalid_argument("Speed must be greater than 0");
  }

  auto currentPosition = getCurrentPosition();

  Robot::Point startPoint{
      static_cast<int>(currentPosition.x),
      static_cast<int>(currentPosition.y)};

  double distance = std::hypot(point.x - startPoint.x, point.y - startPoint.y);
  int numSteps = static_cast<int>(std::round(distance / speed * 100));

  for (int step = 1; step <= numSteps; ++step) {
    double t = static_cast<double>(step) / numSteps;
    Robot::Point newPosition{
        static_cast<int>(startPoint.x + (point.x - startPoint.x) * t),
        static_cast<int>(startPoint.y + (point.y - startPoint.y) * t)};

    Mouse::Move(newPosition);

    Robot::delay(delay);
  }

  // Ensure the final position is accurate
  Mouse::Move(point);
}

Robot::Point Mouse::GetPosition() {
  // TODO: how long exactly should we wait?
  Robot::delay(16);

  Robot::Point point;
#ifdef _WIN32
  POINT cursor = getCurrentPosition();
#elif __APPLE__
  CGPoint cursor = getCurrentPosition();
#endif
  point.x = cursor.x;
  point.y = cursor.y;

  return point;
}

void Mouse::ToggleButton(bool down, MouseButton button, bool doubleClick) {
#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags =
      (button == MouseButton::LEFT_BUTTON
           ? (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP)
       : button == MouseButton::RIGHT_BUTTON
           ? (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP)
           : (down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP));
  SendInput(1, &input, sizeof(INPUT));
#elif __APPLE__
  CGPoint currentPosition = getCurrentPosition();

  CGEventType buttonType;
  switch (button) {
    case MouseButton::LEFT_BUTTON:
      buttonType = down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
      break;
    case MouseButton::RIGHT_BUTTON:
      buttonType = down ? kCGEventRightMouseDown : kCGEventRightMouseUp;
      break;
    case MouseButton::CENTER_BUTTON:
      buttonType = down ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
      break;
  }

  CGEventRef buttonEvent = CGEventCreateMouseEvent(
      nullptr,
      buttonType,
      currentPosition,
      (button == MouseButton::CENTER_BUTTON) ? kCGMouseButtonCenter
                                             : kCGMouseButtonLeft
  );

  if (doubleClick) {
    CGEventSetIntegerValueField(buttonEvent, kCGMouseEventClickState, 2);
  }

  CGEventPost(kCGSessionEventTap, buttonEvent);
  CFRelease(buttonEvent);
#endif
}

void Mouse::Click(MouseButton button) {
  ToggleButton(true, button);
  ToggleButton(false, button);
}

void Mouse::DoubleClick(MouseButton button) {
  ToggleButton(true, button, true);
  ToggleButton(false, button, false);
}

void Mouse::ScrollBy(int y, int x) {
#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = MOUSEEVENTF_WHEEL;
  input.mi.mouseData = static_cast<DWORD>(y);
  SendInput(1, &input, sizeof(INPUT));

  input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
  input.mi.mouseData = static_cast<DWORD>(x);
  SendInput(1, &input, sizeof(INPUT));
#elif __APPLE__
  CGEventRef scrollEvent =
      CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, y, x);
  CGEventPost(kCGHIDEventTap, scrollEvent);
  CFRelease(scrollEvent);
#endif
}

void Mouse::Drag(Robot::Point point, double speed) {
  Mouse::ToggleButton(true, MouseButton::LEFT_BUTTON);
  Mouse::MoveSmooth(point, speed);
  Mouse::ToggleButton(false, MouseButton::LEFT_BUTTON);
}

}  // namespace Robot

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
  const double step = 5.0;  // Change this value to adjust the granularity of the movement
  Robot::Point currentPosition = GetPosition();
  double distance = currentPosition.Distance(point);

  if (distance == 0) {
    return;
  }

  int steps = static_cast<int>(distance / step);
  double dx = (point.x - currentPosition.x) / steps;
  double dy = (point.y - currentPosition.y) / steps;

  int delayTime = static_cast<int>(1000 / speed);

  for (int i = 0; i < steps; ++i) {
    currentPosition.x += dx;
    currentPosition.y += dy;
    Move(currentPosition);
    std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
  }

  // Move to the exact target position in case of small errors during movement
  Move(point);
  return;
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

}  // namespace Robot

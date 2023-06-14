#include "./Mouse.h"
#include "./Utils.h"

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace Robot {

unsigned int Mouse::delay = 16;
bool Mouse::isPressed = false;
MouseButton Mouse::pressedButton = MouseButton::LEFT_BUTTON;

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

  if (Mouse::isPressed) {
    Mouse::MoveWithButtonPressed(point, Mouse::pressedButton);
  }
#endif
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

  CGEventPost(kCGHIDEventTap, buttonEvent);
  CFRelease(buttonEvent);
#endif

  if (down) {
    Mouse::isPressed = true;
    Mouse::pressedButton = button;
  } else {
    Mouse::isPressed = false;
  }
}

void Mouse::MoveWithButtonPressed(Robot::Point point, MouseButton button) {
#ifdef _WIN32
  // On Windows, just calling Move is enough as it will keep the button state.
  Mouse::Move(point);
#elif __APPLE__
  CGPoint target = CGPointMake(point.x, point.y);

  CGEventType dragEventType;
  CGMouseButton cgButton;
  switch (button) {
    case MouseButton::LEFT_BUTTON:
      dragEventType = kCGEventLeftMouseDragged;
      cgButton = kCGMouseButtonLeft;
      break;
    case MouseButton::RIGHT_BUTTON:
      dragEventType = kCGEventRightMouseDragged;
      cgButton = kCGMouseButtonRight;
      break;
    case MouseButton::CENTER_BUTTON:
      dragEventType = kCGEventOtherMouseDragged;
      cgButton = kCGMouseButtonCenter;
      break;
  }

  CGEventRef mouseDragEvent =
      CGEventCreateMouseEvent(nullptr, dragEventType, target, cgButton);
  CGEventPost(kCGHIDEventTap, mouseDragEvent);
  CFRelease(mouseDragEvent);
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

void Mouse::Drag(Robot::Point toPoint) {
  Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
  Robot::delay(10);
  Mouse::Move(toPoint);
  Robot::delay(10);
  Mouse::ToggleButton(false, MouseButton::LEFT_BUTTON);
}

void Mouse::MoveSmooth(Robot::Point point) {
  Robot::Point currentPosition = GetPosition();

  int dx = point.x - currentPosition.x;
  int dy = point.y - currentPosition.y;

  int steps = std::max(std::abs(dx), std::abs(dy));

  float deltaX = static_cast<float>(dx) / steps;
  float deltaY = static_cast<float>(dy) / steps;

  for (int i = 1; i <= steps; i++) {
    Robot::Point stepPosition;
    stepPosition.x = currentPosition.x + static_cast<int>(deltaX * i);
    stepPosition.y = currentPosition.y + static_cast<int>(deltaY * i);

    if (Mouse::isPressed) {
      MoveWithButtonPressed(stepPosition, Mouse::pressedButton);
    } else {
      Move(stepPosition);
    }

    Robot::delay(1);
  }
}

void Mouse::DragSmooth(Robot::Point toPoint) {
  Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
  Robot::delay(10);
  Mouse::MoveSmooth(toPoint);
  Robot::delay(10);
  Mouse::ToggleButton(false, MouseButton::LEFT_BUTTON);
}

}  // namespace Robot

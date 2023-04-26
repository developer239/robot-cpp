#include "Mouse.h"

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace Robot {

void Mouse::moveMouse(Robot::Point point) {
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

void Mouse::dragMouse(Robot::Point point, const MouseButton button) {
  // To be implemented
}

bool Mouse::smoothlyMoveMouse(Robot::Point point, double speed) {
  // To be implemented
}

Robot::Point Mouse::getMousePos() {
  Robot::Point point;

#ifdef _WIN32
  POINT winPoint;
  GetCursorPos(&winPoint);
  point.x = winPoint.x;
  point.y = winPoint.y;
#elif __APPLE__
  CGEventRef event = CGEventCreate(NULL);
  CGPoint location = CGEventGetLocation(event);
  point.x = location.x;
  point.y = location.y;
  CFRelease(event);
#endif

  return point;
}

void Mouse::toggleMouse(bool down, MouseButton button) {
  // To be implemented
}

void Mouse::clickMouse(MouseButton button) {
  // To be implemented
}

void Mouse::doubleClick(MouseButton button) {
  // To be implemented
}

void Mouse::scrollMouse(int x, int y) {
  // To be implemented
}

}  // namespace Robot

#include <Mouse.h>
#include <Utils.h>

int main() {
  Robot::Mouse::Move({400, 200});
  Robot::delay(10);
  Robot::Mouse::Drag({400, 400});

  //  Robot::Mouse::Move({400, 200});

  //  Robot::delay(200);
  //
  //  Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
  //
  //  Robot::delay(200);
  //
  //  Robot::Mouse::Move({400, 400});
  //
  //  Robot::delay(200);
  //
  //  Robot::Mouse::ToggleButton(false, Robot::MouseButton::LEFT_BUTTON);
  //
  //  Robot::delay(200);

  return 0;
}

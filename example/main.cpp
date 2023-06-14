#include <Mouse.h>
#include <Utils.h>

int main() {
  Robot::Mouse::Move({400, 200});
  Robot::delay(10);
  Robot::Mouse::DragSmooth({400, 400});

  return 0;
}

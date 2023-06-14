#include <iostream>
#include <Utils.h>
#include <Screen.h>

int main() {
  Robot::Screen screen;
  auto size = screen.GetScreenSize();
  screen.Capture(0, 0, size.width, size.height);
  screen.SaveAsPNG("screenshot.png");
}

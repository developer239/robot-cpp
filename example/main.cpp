#include <EventHook.h>
#include <Utils.h>
#include <iostream>

int main() {
  using namespace Robot;

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  std::cout << "Pressing and holding SHIFT key" << std::endl;
  Keyboard::HoldStart(Keyboard::SHIFT); // Start holding SHIFT key

  std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for 0.5 seconds

  std::cout << "Pressing and holding 'A' key" << std::endl;
  Keyboard::HoldStart('a'); // Start holding 'A' key

  std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for 2 seconds

  std::cout << "Releasing SHIFT key" << std::endl;
  Keyboard::HoldStop(Keyboard::SHIFT); // Release SHIFT key

  std::this_thread::sleep_for(std::chrono::milliseconds(500)); // wait for 500 ms to ensure SHIFT key is fully released

  std::cout << "Releasing 'a' key" << std::endl;
  Keyboard::HoldStop('a'); // Release 'A' key

  return 0;
}


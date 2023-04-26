#include "./Utils.h"

namespace Robot {

void delay(unsigned int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}  // namespace Robot

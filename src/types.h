#pragma once

#include <cmath>

namespace Robot {

struct Point {
  int x;
  int y;

  [[nodiscard]] double Distance(Point target) const {
    return sqrt(pow(target.x - x, 2) + pow(target.y - y, 2));
  }
};

}  // namespace Robot

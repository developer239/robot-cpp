#include <print>

#include "robot/Robot.h"

// Captures the primary monitor at native pixel resolution and writes a PNG,
// showing the density-correct capture model: the image dimensions are physical
// pixels, so a Retina/HiDPI display yields a larger-than-logical image.
int main() {
  auto session = robot::Session::create();
  if (!session) {
    std::println("Cannot start: {}", session.error().message);
    return 1;
  }

  robot::Screen& screen = (*session)->screen();

  auto primary = screen.primaryMonitor();
  if (!primary) {
    std::println("No primary monitor: {}", primary.error().message);
    return 1;
  }
  std::println(
      "Primary: {}x{} logical, scale {:.2f}",
      primary->logicalBounds.size.width, primary->logicalBounds.size.height,
      primary->scaleFactor
  );

  auto image = screen.captureMonitor(primary->id);
  if (!image) {
    std::println("Capture failed: {}", image.error().message);
    return 1;
  }
  std::println("Captured {}x{} physical pixels", image->width(), image->height());

  if (auto r = image->savePng("primary.png"); !r) {
    std::println("Save failed: {}", r.error().message);
    return 1;
  }
  std::println("Wrote primary.png");
  return 0;
}
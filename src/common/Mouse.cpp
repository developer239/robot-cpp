#include "robot/Mouse.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <expected>
#include <thread>

#include "robot/backend/IMouseBackend.h"

namespace robot {
namespace {

// One interpolation sample per ~4 logical units of travel keeps short hops cheap
// and long sweeps smooth; the cap bounds work and event volume on huge moves.
constexpr int kMaxDerivedSteps = 200;
constexpr double kLogicalUnitsPerStep = 4.0;
constexpr auto kDragSettle = std::chrono::milliseconds(10);

int deriveSteps(const double distance) {
  const int raw = static_cast<int>(std::lround(distance / kLogicalUnitsPerStep));
  return std::clamp(raw, 1, kMaxDerivedSteps);
}

}  // namespace

std::expected<void, Error> Mouse::move(const LogicalPoint point) {
  return backend_->warpCursor(point);
}

std::expected<void, Error> Mouse::moveSmooth(
    const LogicalPoint point, const MouseMoveOptions& options
) {
  auto from = backend_->cursorPosition();
  if (!from) return std::unexpected(from.error());

  const double dx = point.x - from->x;
  const double dy = point.y - from->y;
  const double distance = std::sqrt(dx * dx + dy * dy);

  const int steps = options.steps > 0 ? options.steps : deriveSteps(distance);
  const auto stepDelay = options.duration / steps;

  for (int i = 1; i <= steps; ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(steps);
    const LogicalPoint p{from->x + dx * t, from->y + dy * t};
    if (auto r = backend_->warpCursor(p); !r) return std::unexpected(r.error());
    if (stepDelay.count() > 0) std::this_thread::sleep_for(stepDelay);
  }
  return {};
}

std::expected<LogicalPoint, Error> Mouse::position() {
  return backend_->cursorPosition();
}

std::expected<void, Error> Mouse::press(const MouseButton button) {
  return backend_->button(button, ButtonAction::Down, 1);
}

std::expected<void, Error> Mouse::release(const MouseButton button) {
  return backend_->button(button, ButtonAction::Up, 1);
}

std::expected<void, Error> Mouse::button(
    const MouseButton button, const ButtonAction action, const int clickCount
) {
  return backend_->button(button, action, clickCount);
}

std::expected<void, Error> Mouse::click(const MouseButton button) {
  if (auto r = backend_->button(button, ButtonAction::Down, 1); !r) return r;
  return backend_->button(button, ButtonAction::Up, 1);
}

std::expected<void, Error> Mouse::doubleClick(const MouseButton button) {
  // The second click reports clickCount == 2 so the OS raises a native
  // double-click rather than treating the two as unrelated single clicks.
  if (auto r = backend_->button(button, ButtonAction::Down, 1); !r) return r;
  if (auto r = backend_->button(button, ButtonAction::Up, 1); !r) return r;
  if (auto r = backend_->button(button, ButtonAction::Down, 2); !r) return r;
  return backend_->button(button, ButtonAction::Up, 2);
}

std::expected<void, Error> Mouse::drag(
    const LogicalPoint to, const MouseButton button
) {
  if (auto r = backend_->button(button, ButtonAction::Down, 1); !r) return r;
  std::this_thread::sleep_for(kDragSettle);

  // While the button is held, the backend emits drag events for this warp.
  if (auto r = backend_->warpCursor(to); !r) {
    (void)backend_->button(button, ButtonAction::Up, 1);
    return std::unexpected(r.error());
  }
  std::this_thread::sleep_for(kDragSettle);
  return backend_->button(button, ButtonAction::Up, 1);
}

std::expected<void, Error> Mouse::dragSmooth(
    const LogicalPoint to, const MouseButton button,
    const MouseMoveOptions& options
) {
  if (auto r = backend_->button(button, ButtonAction::Down, 1); !r) return r;
  std::this_thread::sleep_for(kDragSettle);

  auto moved = moveSmooth(to, options);
  std::this_thread::sleep_for(kDragSettle);
  auto released = backend_->button(button, ButtonAction::Up, 1);

  if (!moved) return moved;
  return released;
}

std::expected<void, Error> Mouse::scroll(const ScrollDelta delta) {
  return backend_->scroll(delta);
}

}  // namespace robot

#include <chrono>
#include <print>
#include <thread>

#include "robot/Robot.h"

// Records global input for a few seconds, then replays it. Recording is a
// privileged, platform-limited capability, so the example checks canRecordEvents
// and surfaces the exact error (permission, or Wayland unsupported) instead of
// hanging or failing opaquely.
int main() {
  auto session = robot::Session::create();
  if (!session) {
    std::println("Cannot start: {}", session.error().message);
    return 1;
  }

  if (!(*session)->capabilities().canRecordEvents) {
    std::println("Event recording is not available on this backend.");
    return 1;
  }

  robot::Recorder recorder;
  robot::EventTap& tap = (*session)->eventTap();

  std::println("Recording for 5 seconds...");
  std::thread recordThread([&] {
    auto r = tap.start([&](const robot::InputEvent& e) { recorder.capture(e); });
    if (!r) std::println("Tap error: {}", r.error().message);
  });

  std::this_thread::sleep_for(std::chrono::seconds(5));
  tap.stop();
  recordThread.join();

  std::println("Captured {} events. Replaying in 2s...", recorder.events().size());
  std::this_thread::sleep_for(std::chrono::seconds(2));

  if (auto r = recorder.replay(**session); !r) {
    std::println("Replay failed: {}", r.error().message);
    return 1;
  }
  std::println("Replay complete.");
  return 0;
}
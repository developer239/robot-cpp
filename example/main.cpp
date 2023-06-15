#include <Utils.h>
#include <iostream>
#include <ActionRecorder.h>
// Comment out to test on MacOS
#include <EventHookWindows.h>
// Uncomment to test on MacOS
// #include <EventHookMacOS.h>

int main() {
  int recordFor = 10;

  Robot::ActionRecorder recorder;
  Robot::EventHook hook(recorder);

  std::cout << "Start recording actions in 3 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Start recording
  std::cout << "Starting to record actions for " << recordFor << " seconds..." << std::endl;
  std::thread recordingThread([&hook] { hook.StartRecording(); });

  // Sleep for 10 seconds
  std::this_thread::sleep_for(std::chrono::seconds(recordFor));

  // Stop recording
  std::cout << "Stopping recording..." << std::endl;
  hook.StopRecording();
  recordingThread.join();

  // Wait for 5 seconds before replaying
  std::cout << "Replaying actions in 3 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Replay the recorded actions
  std::cout << "Replaying actions..." << std::endl;
  recorder.ReplayActions();

  return 0;
}

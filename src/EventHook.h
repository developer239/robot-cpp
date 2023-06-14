#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include "./ActionRecorder.h"

// TODO: make work on windows
namespace Robot {

class EventHook {
 public:
  explicit EventHook(ActionRecorder& recorder) : recorder(recorder) {}

  void StartRecording() {
    CGEventMask eventMask = (1 << kCGEventMouseMoved) |
                            (1 << kCGEventLeftMouseDragged) |
                            (1 << kCGEventLeftMouseDown) |
                            (1 << kCGEventLeftMouseUp) |
                            (1 << kCGEventKeyDown);

    eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        eventMask,
        EventCallback,
        &recorder
    );

    if (!eventTap) {
      std::cerr << "Failed to create event tap" << std::endl;
      exit(1);
    }

    runLoopSource =
        CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

    CFRunLoopAddSource(
        CFRunLoopGetCurrent(),
        runLoopSource,
        kCFRunLoopCommonModes
    );

    CGEventTapEnable(eventTap, true);

    eventLoop = CFRunLoopGetCurrent();
    CFRunLoopRun();
  }

  void StopRecording() {
    CGEventTapEnable(eventTap, false);
    CFRunLoopRemoveSource(eventLoop, runLoopSource, kCFRunLoopCommonModes);
    CFRunLoopStop(eventLoop);
  }

 private:
  ActionRecorder& recorder;
  CFMachPortRef eventTap;
  CFRunLoopSourceRef runLoopSource;
  CFRunLoopRef eventLoop;

  static CGEventRef EventCallback(
      CGEventTapProxy _proxy, CGEventType type, CGEventRef event, void* userInfo
  ) {
    auto* recorder = static_cast<ActionRecorder*>(userInfo);

    switch (type) {
      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:{
        CGPoint location = CGEventGetLocation(event);
        recorder->RecordMouseMove(location.x, location.y);
        break;
      }
      case kCGEventLeftMouseDown: {
        CGPoint location = CGEventGetLocation(event);
        CGMouseButton button = kCGMouseButtonLeft;
        recorder->RecordPressLeft(location.x, location.y);
        break;
      }
      case kCGEventLeftMouseUp: {
        CGPoint location = CGEventGetLocation(event);
        CGMouseButton button = kCGMouseButtonLeft;
        recorder->RecordReleaseLeft(location.x, location.y);
        break;
      }
      case kCGEventKeyDown: {
        auto keyCode = (CGKeyCode
        )CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        recorder->RecordKeyboardClick(Keyboard::VirtualKeyToAscii(keyCode));
        break;
      }
      default:
        break;
    }

    return event;
  }
};

}  // namespace Robot

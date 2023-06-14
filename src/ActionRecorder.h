#pragma once

#include <chrono>
#include <thread>
#include <vector>
#include "./Keyboard.h"
#include "./Mouse.h"
#include "Utils.h"

// TODO: make work on windows
namespace Robot {

class Action {
 public:
  enum class ActionType {
    MOUSE_MOVE,
    MOUSE_LEFT_PRESS,
    MOUSE_LEFT_RELEASE,
    KEYBOARD_PRESS,
    KEYBOARD_RELEASE,
  };

  explicit Action(ActionType type, std::chrono::milliseconds delay)
      : type(type), delay(delay) {}
  virtual ~Action() = default;

  ActionType type;
  std::chrono::milliseconds delay;
};

class KeyboardAction : public Action {
 public:
  KeyboardAction(ActionType type, uint16_t key, std::chrono::milliseconds delay)
      : Action(type, delay), key(key) {}

  uint16_t key;
};

class KeyboardPressAction : public KeyboardAction {
 public:
  KeyboardPressAction(uint16_t key, std::chrono::milliseconds delay)
      : KeyboardAction(ActionType::KEYBOARD_PRESS, key, delay) {}
};

class KeyboardReleaseAction : public KeyboardAction {
 public:
  KeyboardReleaseAction(uint16_t key, std::chrono::milliseconds delay)
      : KeyboardAction(ActionType::KEYBOARD_RELEASE, key, delay) {}
};

class MouseAction : public Action {
 public:
  MouseAction(
      ActionType type, float x, float y, std::chrono::milliseconds delay
  )
      : Action(type, delay), x(x), y(y) {}

  float x, y;
};

class MouseLeftPressAction : public MouseAction {
 public:
  MouseLeftPressAction(float x, float y, std::chrono::milliseconds delay)
      : MouseAction(Action::ActionType::MOUSE_LEFT_PRESS, x, y, delay) {}
};

class MouseLeftReleaseAction : public MouseAction {
 public:
  MouseLeftReleaseAction(float x, float y, std::chrono::milliseconds delay)
      : MouseAction(Action::ActionType::MOUSE_LEFT_RELEASE, x, y, delay) {}
};

class ActionRecorder {
 public:
  ActionRecorder() = default;

  void RecordPressLeft(float x, float y) {
    actions.emplace_back(
        std::make_unique<MouseLeftPressAction>(x, y, GetAccumulatedDelay())
    );
  }

  void RecordReleaseLeft(float x, float y) {
    actions.emplace_back(
        std::make_unique<MouseLeftReleaseAction>(x, y, GetAccumulatedDelay())
    );
  }

  void RecordKeyPress(uint16_t key) {
    actions.emplace_back(
        std::make_unique<KeyboardPressAction>(key, GetAccumulatedDelay())
    );
  }

  void RecordKeyRelease(uint16_t key) {
    actions.emplace_back(
        std::make_unique<KeyboardReleaseAction>(key, GetAccumulatedDelay())
    );
  }

  void RecordMouseMove(float x, float y) {
    actions.emplace_back(std::make_unique<MouseAction>(
        Action::ActionType::MOUSE_MOVE,
        x,
        y,
        GetAccumulatedDelay()
    ));
  }

  void ReplayActions() {
    auto replayStartTime = std::chrono::steady_clock::now();

    for (const auto& action : actions) {
      auto targetTime = replayStartTime + action->delay;
      while (std::chrono::steady_clock::now() < targetTime) {
        // Busy-wait for more precise timing control
      }

      switch (action->type) {
        case Action::ActionType::MOUSE_MOVE: {
          auto mouseAction = dynamic_cast<MouseAction*>(action.get());
          if (mouseAction) {
            Robot::Mouse::Move({(int)mouseAction->x, (int)mouseAction->y});
          }
          break;
        }
        case Action::ActionType::MOUSE_LEFT_PRESS: {
          auto mouseLeftPressAction =
              dynamic_cast<MouseLeftPressAction*>(action.get());
          if (mouseLeftPressAction) {
            Robot::Mouse::ToggleButton(true, MouseButton::LEFT_BUTTON);
          }
          break;
        }
        case Action::ActionType::MOUSE_LEFT_RELEASE: {
          auto mouseLeftReleaseAction =
              dynamic_cast<MouseLeftReleaseAction*>(action.get());
          if (mouseLeftReleaseAction) {
            Robot::Mouse::ToggleButton(false, MouseButton::LEFT_BUTTON);
          }
          break;
        }
        case Action::ActionType::KEYBOARD_PRESS: {
          auto keyboardAction = dynamic_cast<KeyboardPressAction*>(action.get());
          if (keyboardAction) {
            char asciiKey = Keyboard::VirtualKeyToAscii(keyboardAction->key);
            if(asciiKey != Keyboard::INVALID_ASCII) {
              Keyboard::Press(asciiKey);
            } else {
              Keyboard::SpecialKey specialKey = Keyboard::VirtualKeyToSpecialKey(keyboardAction->key);
              Keyboard::Press(specialKey);
            }
          }
          break;
        }
        case Action::ActionType::KEYBOARD_RELEASE: {
          auto keyboardAction = dynamic_cast<KeyboardReleaseAction*>(action.get());
          if (keyboardAction) {
            char asciiKey = Keyboard::VirtualKeyToAscii(keyboardAction->key);
            if(asciiKey != Keyboard::INVALID_ASCII) {
              Keyboard::Release(asciiKey);
            } else {
              Keyboard::SpecialKey specialKey = Keyboard::VirtualKeyToSpecialKey(keyboardAction->key);
              Keyboard::Release(specialKey);
            }
          }
          break;
        }
      }
    }
  }

 private:
  std::vector<std::unique_ptr<Action>> actions;
  std::chrono::steady_clock::time_point recordingStartTime =
      std::chrono::steady_clock::now();

  std::chrono::milliseconds GetAccumulatedDelay() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - recordingStartTime
    );
  }
};

}  // namespace Robot

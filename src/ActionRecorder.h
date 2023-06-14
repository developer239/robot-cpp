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
    KEYBOARD_CLICK,
  };

  explicit Action(ActionType type, std::chrono::milliseconds delay)
      : type(type), delay(delay) {}
  virtual ~Action() = default;

  ActionType type;
  std::chrono::milliseconds delay;
};

class KeyboardAction : public Action {
 public:
  KeyboardAction(char key, std::chrono::milliseconds delay)
      : Action(ActionType::KEYBOARD_CLICK, delay), key(key) {}

  char key;
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

  void RecordMouseMove(float x, float y) {
    actions.emplace_back(std::make_unique<MouseAction>(
        Action::ActionType::MOUSE_MOVE,
        x,
        y,
        GetAccumulatedDelay()
    ));
  }

  void RecordKeyboardClick(char key) {
    actions.emplace_back(
        std::make_unique<KeyboardAction>(key, GetAccumulatedDelay())
    );
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
        case Action::ActionType::KEYBOARD_CLICK: {
          auto keyboardAction = dynamic_cast<KeyboardAction*>(action.get());
          if (keyboardAction) {
            Keyboard::Click(keyboardAction->key);
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

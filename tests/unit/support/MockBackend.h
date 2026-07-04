#pragma once

#include <string>
#include <variant>
#include <vector>

#include "robot/backend/IPlatformBackend.h"

// A fully in-memory backend that records every operation and returns programmed
// results. It lets the entire portable stack - chord building, UTF-8 decoding,
// smooth-move sequencing, recorder replay, screen-facade math - be exercised in
// CI with zero OS interaction, which is the whole point of the interface seam.
namespace robot::test {

struct RecordedCall {
  enum class Kind {
    KeyDown, KeyUp, TypeUnicode,
    Warp, CursorPos, Button, Scroll,
    Enumerate, Capture,
  };
  Kind kind;
  Key key = Key::Unknown;
  char32_t codepoint = 0;
  LogicalPoint point;
  MouseButton button = MouseButton::Left;
  ButtonAction action = ButtonAction::Down;
  int clickCount = 0;
  ScrollDelta scroll;
  PhysicalRect region;
};

class MockKeyboard final : public backend::IKeyboardBackend {
 public:
  explicit MockKeyboard(std::vector<RecordedCall>& log) : log_(&log) {}
  std::expected<void, Error> keyDown(Key k) override {
    log_->push_back({.kind = RecordedCall::Kind::KeyDown, .key = k});
    return {};
  }
  std::expected<void, Error> keyUp(Key k) override {
    log_->push_back({.kind = RecordedCall::Kind::KeyUp, .key = k});
    return {};
  }
  std::expected<void, Error> typeUnicode(char32_t cp) override {
    log_->push_back({.kind = RecordedCall::Kind::TypeUnicode, .codepoint = cp});
    return {};
  }
 private:
  std::vector<RecordedCall>* log_;
};

class MockMouse final : public backend::IMouseBackend {
 public:
  explicit MockMouse(std::vector<RecordedCall>& log) : log_(&log) {}
  std::expected<void, Error> warpCursor(LogicalPoint p) override {
    position_ = p;
    log_->push_back({.kind = RecordedCall::Kind::Warp, .point = p});
    return {};
  }
  std::expected<LogicalPoint, Error> cursorPosition() override {
    log_->push_back({.kind = RecordedCall::Kind::CursorPos, .point = position_});
    return position_;
  }
  std::expected<void, Error> button(
      MouseButton b, ButtonAction a, int count
  ) override {
    log_->push_back({.kind = RecordedCall::Kind::Button, .button = b,
                     .action = a, .clickCount = count});
    return {};
  }
  std::expected<void, Error> scroll(ScrollDelta d) override {
    log_->push_back({.kind = RecordedCall::Kind::Scroll, .scroll = d});
    return {};
  }
  void setPosition(LogicalPoint p) { position_ = p; }
 private:
  std::vector<RecordedCall>* log_;
  LogicalPoint position_;
};

class MockScreen final : public backend::IScreenBackend {
 public:
  explicit MockScreen(std::vector<RecordedCall>& log) : log_(&log) {}
  std::expected<std::vector<Monitor>, Error> enumerateMonitors() override {
    log_->push_back({.kind = RecordedCall::Kind::Enumerate});
    return monitors_;
  }
  std::expected<Image, Error> captureRegion(PhysicalRect r) override {
    log_->push_back({.kind = RecordedCall::Kind::Capture, .region = r});
    std::vector<Rgba> px(
        static_cast<std::size_t>(r.size.width) * r.size.height, fill_);
    return Image{r.size, std::move(px)};
  }
  void setMonitors(std::vector<Monitor> m) { monitors_ = std::move(m); }
  void setFill(Rgba c) { fill_ = c; }
 private:
  std::vector<RecordedCall>* log_;
  std::vector<Monitor> monitors_;
  Rgba fill_{1, 2, 3, 255};
};

class MockPlatformBackend final : public backend::IPlatformBackend {
 public:
  MockPlatformBackend()
      : keyboard_(log_), mouse_(log_), screen_(log_) {
    capabilities_.backendName = "Mock";
    capabilities_.canInjectKeyboard = true;
    capabilities_.canInjectMouse = true;
    capabilities_.canTypeUnicode = true;
    capabilities_.canWarpCursor = true;
    capabilities_.canReadCursorPosition = true;
    capabilities_.canEnumerateMonitors = true;
    capabilities_.canCaptureScreen = true;
  }
  backend::IKeyboardBackend& keyboard() override { return keyboard_; }
  backend::IMouseBackend& mouse() override { return mouse_; }
  backend::IScreenBackend& screen() override { return screen_; }
  backend::IEventTapBackend* eventTap() override { return nullptr; }
  const Capabilities& capabilities() const override { return capabilities_; }

  std::vector<RecordedCall>& log() { return log_; }
  MockMouse& mockMouse() { return mouse_; }
  MockScreen& mockScreen() { return screen_; }

 private:
  std::vector<RecordedCall> log_;
  Capabilities capabilities_;
  MockKeyboard keyboard_;
  MockMouse mouse_;
  MockScreen screen_;
};

}  // namespace robot::test
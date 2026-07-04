#include "X11EventTapBackend.h"

#include <X11/Xproto.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>

#include "X11Display.h"
#include "X11KeyMap.h"
#include "robot/Event.h"

namespace robot::x11 {
namespace {

// XRecord hands back raw protocol data; the callback reconstructs high-level
// events. State lives on the backend, reached through the intercept closure.
struct TapState {
  X11EventTapBackend* self = nullptr;
  Display* lookupDisplay = nullptr;  // For keycode -> keysym translation.
  EventSink* sink = nullptr;
};

void interceptCallback(XPointer closure, XRecordInterceptData* data) {
  auto* state = reinterpret_cast<TapState*>(closure);

  if (data->category == XRecordFromServer && data->data != nullptr) {
    const auto* event = reinterpret_cast<const xEvent*>(data->data);
    const std::uint8_t type = event->u.u.type & 0x7F;
    const int detail = event->u.u.detail;
    const std::int16_t rootX = event->u.keyButtonPointer.rootX;
    const std::int16_t rootY = event->u.keyButtonPointer.rootY;
    const LogicalPoint pos{static_cast<double>(rootX),
                           static_cast<double>(rootY)};

    switch (type) {
      case KeyPress:
      case KeyRelease: {
        const KeySym ks = XkbKeycodeToKeysym(
            state->lookupDisplay, static_cast<KeyCode>(detail), 0, 0
        );
        const Key key = keysymToKey(ks);
        if (key != Key::Unknown) {
          (*state->sink)(KeyEvent{key, type == KeyPress});
        }
        break;
      }
      case ButtonPress:
      case ButtonRelease: {
        const bool down = type == ButtonPress;
        switch (detail) {
          case 1:
            (*state->sink)(MouseButtonEvent{MouseButton::Left, down, pos});
            break;
          case 2:
            (*state->sink)(MouseButtonEvent{MouseButton::Middle, down, pos});
            break;
          case 3:
            (*state->sink)(MouseButtonEvent{MouseButton::Right, down, pos});
            break;
          // Wheel arrives as button clicks; emit a scroll on press only.
          case 4:
            if (down) (*state->sink)(ScrollEvent{ScrollDelta::lines(1, 0), pos});
            break;
          case 5:
            if (down) (*state->sink)(ScrollEvent{ScrollDelta::lines(-1, 0), pos});
            break;
          case 6:
            if (down) (*state->sink)(ScrollEvent{ScrollDelta::lines(0, -1), pos});
            break;
          case 7:
            if (down) (*state->sink)(ScrollEvent{ScrollDelta::lines(0, 1), pos});
            break;
          case 8:
            (*state->sink)(MouseButtonEvent{MouseButton::X1, down, pos});
            break;
          case 9:
            (*state->sink)(MouseButtonEvent{MouseButton::X2, down, pos});
            break;
          default:
            break;
        }
        break;
      }
      case MotionNotify:
        (*state->sink)(MouseMoveEvent{pos});
        break;
      default:
        break;
    }
  }

  if (data != nullptr) XRecordFreeData(data);
}

TapState g_state;  // One active tap per process, mirroring the hook model.

}  // namespace

std::expected<void, Error> X11EventTapBackend::start(EventSink sink) {
  sink_ = std::move(sink);

  control_ = XOpenDisplay(nullptr);
  data_ = XOpenDisplay(nullptr);
  if (control_ == nullptr || data_ == nullptr) {
    if (control_ != nullptr) XCloseDisplay(control_);
    if (data_ != nullptr) XCloseDisplay(data_);
    control_ = nullptr;
    data_ = nullptr;
    return std::unexpected(
        Error::backendUnavailable("cannot open X connections for XRecord")
    );
  }

  int major = 0;
  int minor = 0;
  if (XRecordQueryVersion(control_, &major, &minor) == 0) {
    XCloseDisplay(control_);
    XCloseDisplay(data_);
    control_ = nullptr;
    data_ = nullptr;
    return std::unexpected(
        Error::unsupported("the X server lacks the RECORD extension")
    );
  }

  XRecordRange* range = XRecordAllocRange();
  range->device_events.first = KeyPress;
  range->device_events.last = MotionNotify;

  XRecordClientSpec clients = XRecordAllClients;
  auto ctx = XRecordCreateContext(control_, 0, &clients, 1, &range, 1);
  XFree(range);
  if (ctx == 0) {
    XCloseDisplay(control_);
    XCloseDisplay(data_);
    control_ = nullptr;
    data_ = nullptr;
    return std::unexpected(Error::platformError("XRecordCreateContext"));
  }
  context_ = reinterpret_cast<void*>(ctx);

  g_state = TapState{this, control_, &sink_};
  running_.store(true);

  // Blocks, delivering events through interceptCallback, until stop() disables
  // the context from the control connection on another thread.
  if (XRecordEnableContext(
          data_, ctx, interceptCallback,
          reinterpret_cast<XPointer>(&g_state)
      ) == 0) {
    running_.store(false);
    XRecordFreeContext(control_, ctx);
    XCloseDisplay(control_);
    XCloseDisplay(data_);
    control_ = nullptr;
    data_ = nullptr;
    context_ = nullptr;
    return std::unexpected(Error::platformError("XRecordEnableContext"));
  }

  running_.store(false);
  XRecordFreeContext(control_, ctx);
  XCloseDisplay(control_);
  XCloseDisplay(data_);
  control_ = nullptr;
  data_ = nullptr;
  context_ = nullptr;
  return {};
}

void X11EventTapBackend::stop() {
  running_.store(false);
  // Disabling the context from the control connection causes the blocking
  // XRecordEnableContext on the data connection to return. XRecordDisableContext
  // is safe to call from another thread against the control display.
  if (context_ != nullptr && control_ != nullptr) {
    XRecordDisableContext(
        control_, reinterpret_cast<XRecordContext>(context_)
    );
    XFlush(control_);
  }
}

}  // namespace robot::x11
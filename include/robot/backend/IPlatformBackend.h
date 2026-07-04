#pragma once

#include "robot/Capabilities.h"
#include "robot/backend/IEventTapBackend.h"
#include "robot/backend/IKeyboardBackend.h"
#include "robot/backend/IMouseBackend.h"
#include "robot/backend/IScreenBackend.h"

namespace robot::backend {

// Aggregate root for one platform's implementation and the sole owner of its
// sub-backends. A Session owns exactly one IPlatformBackend; the facades borrow
// the sub-backends through it. This is the object a mock replaces to exercise the
// entire portable stack with zero OS interaction.
//
// The sub-backend accessors return references (always present). eventTap()
// returns a pointer because a global tap is genuinely optional per platform build
// - nullptr means "this build cannot record", which the EventTap facade surfaces
// as Unsupported. capabilities() reports the assembled truth about what this
// backend can do in the current environment, computed once at construction.
class IPlatformBackend {
 public:
  virtual ~IPlatformBackend() = default;

  [[nodiscard]] virtual IKeyboardBackend& keyboard() = 0;
  [[nodiscard]] virtual IMouseBackend& mouse() = 0;
  [[nodiscard]] virtual IScreenBackend& screen() = 0;

  // nullptr when this platform build provides no global input tap.
  [[nodiscard]] virtual IEventTapBackend* eventTap() = 0;

  [[nodiscard]] virtual const Capabilities& capabilities() const = 0;
};

}  // namespace robot::backend
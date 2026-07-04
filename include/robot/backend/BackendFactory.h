#pragma once

#include <expected>
#include <memory>

#include "robot/Error.h"
#include "robot/Session.h"  // SessionOptions
#include "robot/backend/IPlatformBackend.h"

namespace robot::backend {

// The single seam between portable code and the operating system. Each platform
// directory (src/platform/{macos,windows,linux}) provides exactly one definition
// of this function, and CMake compiles exactly one of those directories, so at
// link time there is one implementation and no preprocessor branching in any
// translation unit that consumes it.
//
// This function performs all fallible, environment-dependent initialization:
//   * selecting a concrete backend (including the Linux X11-vs-uinput choice and
//     honest failure under an unprivileged Wayland session),
//   * preflighting permissions when SessionOptions requests it (returning
//     ErrorCode::PermissionDenied before any Session exists rather than at first
//     use),
//   * probing the display server and building the Capabilities report.
//
// On success it returns a fully-initialized IPlatformBackend; on failure a
// specific Error. It never returns a partially-constructed backend. Session::
// create() is the only caller.
[[nodiscard]] std::expected<std::unique_ptr<IPlatformBackend>, Error>
createPlatformBackend(const SessionOptions& options);

}  // namespace robot::backend
#include "LinuxPlatformBackend.h"

#include <utility>

#include "NullScreenBackend.h"

namespace robot::linux_backend {

X11PlatformBackend::X11PlatformBackend(
    x11::X11Connection connection, Capabilities capabilities
)
    : connection_(std::move(connection)),
      capabilities_(std::move(capabilities)),
      keyboard_(std::make_unique<x11::X11KeyboardBackend>(connection_)),
      mouse_(std::make_unique<x11::X11MouseBackend>(connection_)),
      screen_(std::make_unique<x11::X11ScreenBackend>(connection_)),
      eventTap_(std::make_unique<x11::X11EventTapBackend>()) {}

UinputPlatformBackend::UinputPlatformBackend(
    std::unique_ptr<linux_uinput::UinputBackend> device, Capabilities capabilities
)
    : device_(std::move(device)),
      screen_(std::make_unique<NullScreenBackend>()),
      capabilities_(std::move(capabilities)) {}

}  // namespace robot::linux_backend
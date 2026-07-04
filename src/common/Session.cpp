#include "robot/Session.h"

#include <utility>

#include "robot/backend/BackendFactory.h"
#include "robot/backend/IPlatformBackend.h"

namespace robot {

std::expected<std::unique_ptr<Session>, Error> Session::create(
    const SessionOptions& options
) {
  auto backend = backend::createPlatformBackend(options);
  if (!backend) return std::unexpected(backend.error());
  return std::make_unique<Session>(PrivateTag{}, std::move(*backend));
}

// backend_ is declared first, so it is fully constructed before the facades'
// initializers run and can safely hand out references into it.
Session::Session(
    PrivateTag, std::unique_ptr<backend::IPlatformBackend> backend
)
    : backend_(std::move(backend)),
      capabilities_(backend_->capabilities()),
      keyboard_(backend_->keyboard()),
      mouse_(backend_->mouse()),
      screen_(backend_->screen()),
      eventTap_(backend_->eventTap()) {}

// Defined here, where IPlatformBackend is complete, so unique_ptr can destroy it.
Session::~Session() = default;

}  // namespace robot
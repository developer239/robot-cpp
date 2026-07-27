#include "robot/Windows.h"

#include <cmath>
#include <format>
#include <utility>

#include "robot/backend/IWindowBackend.h"

namespace robot {

// ── WindowStream facade ─────────────────────────────────────────────────────

WindowStream::WindowStream(
    std::unique_ptr<backend::IWindowStreamBackend> impl
)
    : impl_(std::move(impl)) {}

WindowStream::~WindowStream() {
  if (impl_ != nullptr) impl_->stop();
}

WindowStream::WindowStream(WindowStream&&) noexcept = default;
WindowStream& WindowStream::operator=(WindowStream&&) noexcept = default;

void WindowStream::stop() {
  if (impl_ != nullptr) impl_->stop();
}

bool WindowStream::isRunning() const {
  return impl_ != nullptr && impl_->isRunning();
}

// ── Windows facade ──────────────────────────────────────────────────────────

namespace {

constexpr std::string_view kNoBackend =
    "window enumeration/capture is not implemented on this platform build";

}  // namespace

std::expected<std::vector<Window>, Error> Windows::list() {
  if (backend_ == nullptr) return std::unexpected(Error::unsupported(kNoBackend));
  return backend_->enumerateWindows();
}

std::expected<LogicalRect, Error> Windows::frame(const std::uint32_t windowId) {
  if (backend_ == nullptr) return std::unexpected(Error::unsupported(kNoBackend));
  return backend_->windowFrame(windowId);
}

std::expected<std::optional<std::uint32_t>, Error> Windows::topmostWindowId() {
  if (backend_ == nullptr) return std::unexpected(Error::unsupported(kNoBackend));
  return backend_->topmostWindowId();
}

std::expected<void, Error> Windows::activateApplicationFor(
    const std::uint32_t windowId
) {
  if (backend_ == nullptr) return std::unexpected(Error::unsupported(kNoBackend));
  return backend_->activateApplicationForWindow(windowId);
}

std::expected<WindowStream, Error> Windows::stream(
    const std::uint32_t windowId, const WindowStreamOptions& options,
    VideoFrameCallback onFrame, StreamErrorCallback onError
) {
  if (backend_ == nullptr) return std::unexpected(Error::unsupported(kNoBackend));
  if (onFrame == nullptr) {
    return std::unexpected(
        Error::invalidArgument("stream() requires a frame callback")
    );
  }
  if (!std::isfinite(options.maxFps) || options.maxFps < 1.0 ||
      options.maxFps > 1000.0) {
    return std::unexpected(Error::invalidArgument(std::format(
        "maxFps must be a finite value in [1, 1000], got {}", options.maxFps
    )));
  }

  auto stream = backend_->startStream(
      windowId, options, std::move(onFrame), std::move(onError)
  );
  if (!stream) return std::unexpected(stream.error());
  return WindowStream(std::move(*stream));
}

}  // namespace robot

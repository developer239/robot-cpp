#pragma once

#include <cstdint>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>

namespace robot {

// Stable, exhaustive set of failure categories the library can report. Every
// fallible API returns std::expected<T, Error>; callers switch on ErrorCode for
// programmatic handling and read message for human-readable detail.
enum class ErrorCode : std::uint16_t {
  // The active backend cannot perform this operation on this platform/session
  // (for example warping the cursor under Wayland). This is a hard, reported
  // limitation, never a silent fallback.
  Unsupported,
  // A required OS permission is not granted: macOS Accessibility or Screen
  // Recording, Linux input-group / uinput access, and so on.
  PermissionDenied,
  // No usable backend could be initialized for the current environment.
  BackendUnavailable,
  // An argument was out of range or otherwise malformed.
  InvalidArgument,
  // A referenced monitor id does not exist.
  MonitorNotFound,
  // A character or key cannot be represented by the active keyboard layout or
  // injection method.
  UnmappableInput,
  // A capture, encode, or I/O step failed.
  CaptureFailed,
  EncodeFailed,
  IoError,
  // An underlying OS call failed for a reason the library did not classify;
  // message carries the platform detail.
  PlatformError,
};

constexpr std::string_view toString(const ErrorCode code) {
  switch (code) {
    case ErrorCode::Unsupported: return "Unsupported";
    case ErrorCode::PermissionDenied: return "PermissionDenied";
    case ErrorCode::BackendUnavailable: return "BackendUnavailable";
    case ErrorCode::InvalidArgument: return "InvalidArgument";
    case ErrorCode::MonitorNotFound: return "MonitorNotFound";
    case ErrorCode::UnmappableInput: return "UnmappableInput";
    case ErrorCode::CaptureFailed: return "CaptureFailed";
    case ErrorCode::EncodeFailed: return "EncodeFailed";
    case ErrorCode::IoError: return "IoError";
    case ErrorCode::PlatformError: return "PlatformError";
  }
  std::abort();
}

// One error type for the whole library. Construct through the named factories
// so every message has a consistent, actionable shape; never brace-init at a
// call site.
struct Error {
  ErrorCode code;
  std::string message;

  static Error unsupported(std::string_view what) {
    return {ErrorCode::Unsupported,
            std::format("Unsupported operation: {}", what)};
  }
  static Error permissionDenied(std::string_view what) {
    return {ErrorCode::PermissionDenied,
            std::format("Permission denied: {}", what)};
  }
  static Error backendUnavailable(std::string_view detail) {
    return {ErrorCode::BackendUnavailable,
            std::format("No usable backend: {}", detail)};
  }
  static Error invalidArgument(std::string_view detail) {
    return {ErrorCode::InvalidArgument,
            std::format("Invalid argument: {}", detail)};
  }
  static Error monitorNotFound(const std::uint32_t id) {
    return {ErrorCode::MonitorNotFound, std::format("Monitor {} not found", id)};
  }
  static Error unmappableInput(std::string_view detail) {
    return {ErrorCode::UnmappableInput,
            std::format("Cannot map input: {}", detail)};
  }
  static Error captureFailed(std::string_view detail) {
    return {ErrorCode::CaptureFailed,
            std::format("Screen capture failed: {}", detail)};
  }
  static Error encodeFailed(std::string_view detail) {
    return {ErrorCode::EncodeFailed, std::format("Encode failed: {}", detail)};
  }
  static Error ioError(std::string_view detail) {
    return {ErrorCode::IoError, std::format("I/O error: {}", detail)};
  }
  static Error platformError(std::string_view api, const long osStatus) {
    return {ErrorCode::PlatformError,
            std::format("{} failed (status {})", api, osStatus)};
  }
  static Error platformError(std::string_view detail) {
    return {ErrorCode::PlatformError, std::string{detail}};
  }
};

}  // namespace robot

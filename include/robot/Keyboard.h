#pragma once

#include <chrono>
#include <expected>
#include <string_view>

#include "robot/Error.h"
#include "robot/Key.h"
#include "robot/Modifiers.h"

namespace robot {
namespace backend {
class IKeyboardBackend;
}

// Options for human-like typing. Per-character gaps are drawn from a normal
// distribution clamped to [min, max] so the pacing looks organic but never
// stalls or goes negative. Determinism is available by fixing the seed.
struct HumanTypingOptions {
  std::chrono::milliseconds meanDelay{75};
  std::chrono::milliseconds stddev{25};
  std::chrono::milliseconds minDelay{15};
  std::chrono::milliseconds maxDelay{250};
  // 0 selects a nondeterministic seed; any other value makes the timing (and
  // thus tests) reproducible.
  std::uint64_t seed = 0;
};

// Keyboard input, split cleanly into two models that must not be confused:
//
//   Physical keys (press / release / tap): drive a key by position via the Key
//   enum. Correct for shortcuts, chords, and games (Control+C, WASD). What
//   character a key emits depends on the user's active layout, which is the
//   point - a positional action stays positional across layouts.
//
//   Text (typeChar / typeText): inject Unicode directly, independent of layout.
//   This is the only correct way to produce specific characters - capitals,
//   symbols, accented letters, CJK - because it does not assume a US keyboard or
//   any particular key-to-character mapping. If a backend cannot inject Unicode
//   (canTypeUnicode == false) these fail with ErrorCode::Unsupported rather than
//   guessing at keystrokes.
//
// Obtained from Session::keyboard(); never constructed directly by users. Holds
// a reference into the Session-owned backend and does not own it.
class Keyboard {
 public:
  explicit Keyboard(backend::IKeyboardBackend& backend) : backend_(&backend) {}

  // Atomic physical key transitions. LeftShift, RightAlt, etc. are Keys, so
  // holding a modifier down manually is just press(Key::LeftShift).
  [[nodiscard]] std::expected<void, Error> press(Key key);
  [[nodiscard]] std::expected<void, Error> release(Key key);

  // Press key (optionally with modifiers held), then release everything in
  // reverse order. This is the chord primitive: tap(Key::C, Modifier::Meta)
  // performs Command/Ctrl+C. Modifiers are applied as physical modifier keys.
  [[nodiscard]] std::expected<void, Error> tap(
      Key key, Modifiers modifiers = {}
  );

  // Inject a single Unicode scalar value as text (layout-independent).
  [[nodiscard]] std::expected<void, Error> typeChar(char32_t codepoint);

  // Inject UTF-8 text as a sequence of Unicode scalars. Invalid UTF-8 is
  // rejected with InvalidArgument rather than typing replacement characters.
  [[nodiscard]] std::expected<void, Error> typeText(std::string_view utf8);

  // As typeText, with a randomized inter-character delay (see HumanTypingOptions).
  [[nodiscard]] std::expected<void, Error> typeTextHumanLike(
      std::string_view utf8, const HumanTypingOptions& options = {}
  );

 private:
  backend::IKeyboardBackend* backend_;
};

}  // namespace robot
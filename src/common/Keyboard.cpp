#include "robot/Keyboard.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <expected>
#include <random>
#include <thread>
#include <vector>

#include "robot/backend/IKeyboardBackend.h"

namespace robot {
namespace {

// A logical Modifier is realized as a physical modifier key. Left variants are
// used by convention; a caller wanting a specific side presses that Key directly.
Key modifierToKey(const Modifier m) {
  switch (m) {
    case Modifier::Shift: return Key::LeftShift;
    case Modifier::Control: return Key::LeftControl;
    case Modifier::Alt: return Key::LeftAlt;
    case Modifier::Meta: return Key::LeftMeta;
    case Modifier::CapsLock: return Key::CapsLock;
  }
  std::abort();
}

// Decode UTF-8 to Unicode scalar values, rejecting malformed input rather than
// substituting replacement characters (typing a replacement character silently
// would be exactly the kind of best-effort behaviour this library avoids).
std::expected<std::vector<char32_t>, Error> decodeUtf8(std::string_view s) {
  std::vector<char32_t> out;
  out.reserve(s.size());

  const std::size_t n = s.size();
  const auto byte = [&](const std::size_t idx) {
    return static_cast<unsigned char>(s[idx]);
  };

  std::size_t i = 0;
  while (i < n) {
    const unsigned char lead = byte(i);
    char32_t cp = 0;
    std::size_t extra = 0;
    char32_t minValue = 0;

    if (lead < 0x80) {
      cp = lead;
      extra = 0;
      minValue = 0;
    } else if ((lead & 0xE0) == 0xC0) {
      cp = lead & 0x1F;
      extra = 1;
      minValue = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
      cp = lead & 0x0F;
      extra = 2;
      minValue = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
      cp = lead & 0x07;
      extra = 3;
      minValue = 0x10000;
    } else {
      return std::unexpected(Error::invalidArgument("invalid UTF-8 lead byte"));
    }

    if (i + extra >= n) {
      return std::unexpected(
          Error::invalidArgument("truncated UTF-8 sequence")
      );
    }
    for (std::size_t k = 1; k <= extra; ++k) {
      const unsigned char cont = byte(i + k);
      if ((cont & 0xC0) != 0x80) {
        return std::unexpected(
            Error::invalidArgument("invalid UTF-8 continuation byte")
        );
      }
      cp = (cp << 6) | (cont & 0x3F);
    }

    if (cp < minValue) {
      return std::unexpected(Error::invalidArgument("overlong UTF-8 encoding"));
    }
    if (cp > 0x10FFFF) {
      return std::unexpected(Error::invalidArgument("codepoint out of range"));
    }
    if (cp >= 0xD800 && cp <= 0xDFFF) {
      return std::unexpected(
          Error::invalidArgument("UTF-16 surrogate is not a scalar value")
      );
    }

    out.push_back(cp);
    i += extra + 1;
  }
  return out;
}

bool isScalarValue(const char32_t cp) {
  return cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF);
}

}  // namespace

std::expected<void, Error> Keyboard::press(const Key key) {
  return backend_->keyDown(key);
}

std::expected<void, Error> Keyboard::release(const Key key) {
  return backend_->keyUp(key);
}

std::expected<void, Error> Keyboard::tap(
    const Key key, const Modifiers modifiers
) {
  // Press modifiers in a stable order, then the key, then unwind in reverse. On
  // any failure, already-pressed keys are released so no modifier is left stuck.
  const Modifier order[] = {Modifier::Control, Modifier::Alt, Modifier::Shift,
                            Modifier::Meta, Modifier::CapsLock};

  std::vector<Key> pressed;
  for (const Modifier m : order) {
    if (!modifiers.has(m)) continue;
    const Key mk = modifierToKey(m);
    if (auto r = backend_->keyDown(mk); !r) {
      for (auto it = pressed.rbegin(); it != pressed.rend(); ++it) {
        (void)backend_->keyUp(*it);
      }
      return std::unexpected(r.error());
    }
    pressed.push_back(mk);
  }

  std::expected<void, Error> result = backend_->keyDown(key);
  if (result) result = backend_->keyUp(key);

  for (auto it = pressed.rbegin(); it != pressed.rend(); ++it) {
    if (auto r = backend_->keyUp(*it); !r && result) {
      result = std::unexpected(r.error());
    }
  }
  return result;
}

std::expected<void, Error> Keyboard::typeChar(const char32_t codepoint) {
  if (!isScalarValue(codepoint)) {
    return std::unexpected(
        Error::invalidArgument("not a valid Unicode scalar value")
    );
  }
  return backend_->typeUnicode(codepoint);
}

std::expected<void, Error> Keyboard::typeText(const std::string_view utf8) {
  auto codepoints = decodeUtf8(utf8);
  if (!codepoints) return std::unexpected(codepoints.error());
  for (const char32_t cp : *codepoints) {
    if (auto r = backend_->typeUnicode(cp); !r) {
      return std::unexpected(r.error());
    }
  }
  return {};
}

std::expected<void, Error> Keyboard::typeTextHumanLike(
    const std::string_view utf8, const HumanTypingOptions& options
) {
  auto codepoints = decodeUtf8(utf8);
  if (!codepoints) return std::unexpected(codepoints.error());

  std::mt19937_64 engine(
      options.seed == 0 ? std::random_device{}() : options.seed
  );
  std::normal_distribution<double> distribution(
      static_cast<double>(options.meanDelay.count()),
      static_cast<double>(options.stddev.count())
  );
  const long long lo = options.minDelay.count();
  const long long hi = options.maxDelay.count();

  for (const char32_t cp : *codepoints) {
    if (auto r = backend_->typeUnicode(cp); !r) {
      return std::unexpected(r.error());
    }
    long long ms = std::lround(distribution(engine));
    ms = std::clamp(ms, lo, hi);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
  return {};
}

}  // namespace robot
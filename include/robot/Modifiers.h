#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

namespace robot {

// A modifier considered by its role, not its physical side. Left/right variants
// are distinct physical keys (see Key); Modifier is the logical intent used to
// build chords and to report which modifiers were active for an event.
enum class Modifier : std::uint8_t {
  Shift = 0,
  Control = 1,
  Alt = 2,   // Option on macOS.
  Meta = 3,  // Command on macOS, Windows logo on Windows, Super on Linux.
  CapsLock = 4,
};

constexpr std::string_view toString(const Modifier modifier) {
  switch (modifier) {
    case Modifier::Shift: return "Shift";
    case Modifier::Control: return "Control";
    case Modifier::Alt: return "Alt";
    case Modifier::Meta: return "Meta";
    case Modifier::CapsLock: return "CapsLock";
  }
  std::abort();
}

// An immutable set of modifiers stored as a small bitmask. Value semantics,
// cheap to copy. Build with operator| on Modifier, or Modifiers{m}.with(...).
class Modifiers {
 public:
  constexpr Modifiers() = default;
  constexpr explicit Modifiers(const Modifier m) : bits_(bit(m)) {}

  [[nodiscard]] constexpr bool has(const Modifier m) const {
    return (bits_ & bit(m)) != 0;
  }
  [[nodiscard]] constexpr bool empty() const { return bits_ == 0; }
  [[nodiscard]] constexpr std::uint8_t bits() const { return bits_; }

  constexpr Modifiers& operator|=(const Modifier m) {
    bits_ |= bit(m);
    return *this;
  }
  constexpr Modifiers& operator|=(const Modifiers other) {
    bits_ |= other.bits_;
    return *this;
  }

  [[nodiscard]] constexpr Modifiers with(const Modifier m) const {
    Modifiers result = *this;
    result |= m;
    return result;
  }
  [[nodiscard]] constexpr Modifiers without(const Modifier m) const {
    Modifiers result = *this;
    result.bits_ &= static_cast<std::uint8_t>(~bit(m));
    return result;
  }

  friend constexpr bool operator==(Modifiers, Modifiers) = default;

 private:
  static constexpr std::uint8_t bit(const Modifier m) {
    return static_cast<std::uint8_t>(1u << static_cast<std::uint8_t>(m));
  }

  std::uint8_t bits_ = 0;
};

[[nodiscard]] constexpr Modifiers operator|(const Modifier a, const Modifier b) {
  return Modifiers{a}.with(b);
}
[[nodiscard]] constexpr Modifiers operator|(const Modifiers a, const Modifier b) {
  return a.with(b);
}

// Human-readable form, for example "Control+Shift". An empty set renders as "".
inline std::string toString(const Modifiers modifiers) {
  std::string result;
  const Modifier order[] = {Modifier::Control, Modifier::Alt, Modifier::Shift,
                            Modifier::Meta, Modifier::CapsLock};
  for (const Modifier m : order) {
    if (!modifiers.has(m)) continue;
    if (!result.empty()) result += '+';
    result += toString(m);
  }
  return result;
}

}  // namespace robot

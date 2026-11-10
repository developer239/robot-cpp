#include "MacKeyLayout.h"

#include <Carbon/Carbon.h>

#include <array>
#include <mutex>
#include <string>
#include <unordered_map>

namespace robot::mac {
namespace {

struct Layer {
  UInt32 carbonModifiers;
  CGEventFlags flags;
};

// Fewest modifiers first, so a character a plain key types never goes out with
// Shift or Option held, and one Shift types never with Option.
constexpr std::array<Layer, 4> kLayers{{
    {0, 0},
    {(shiftKey >> 8) & 0xFF, kCGEventFlagMaskShift},
    {(optionKey >> 8) & 0xFF, kCGEventFlagMaskAlternate},
    {((shiftKey | optionKey) >> 8) & 0xFF,
     kCGEventFlagMaskShift | kCGEventFlagMaskAlternate},
}};

// Virtual key codes run below 128; the keypad's codes sit above the main
// block's, so a digit or a period maps to its main key.
constexpr UInt16 kKeycodeCount = 128;

struct LayoutMap {
  std::string sourceId;
  std::unordered_map<char32_t, KeyStroke> strokes;
};

std::string inputSourceId(TISInputSourceRef source) {
  const auto id = static_cast<CFStringRef>(
      TISGetInputSourceProperty(source, kTISPropertyInputSourceID)
  );
  if (id == nullptr) return {};
  std::array<char, 256> buffer{};
  if (!CFStringGetCString(
          id, buffer.data(), buffer.size(), kCFStringEncodingUTF8
      )) {
    return {};
  }
  return buffer.data();
}

// Control characters and the private-use block (where macOS puts the arrow and
// function keys) are not text, so the keys that produce them are not mapped.
bool isText(const char32_t c) {
  return c >= 0x20 && c != 0x7F && !(c >= 0xE000 && c <= 0xF8FF);
}

std::optional<char32_t> singleScalar(
    const UniChar* units, const UniCharCount n
) {
  if (n == 1) return units[0];
  const bool isPair = n == 2 && units[0] >= 0xD800 && units[0] <= 0xDBFF &&
                      units[1] >= 0xDC00 && units[1] <= 0xDFFF;
  if (isPair) {
    return 0x10000 + ((static_cast<char32_t>(units[0]) - 0xD800) << 10) +
           (static_cast<char32_t>(units[1]) - 0xDC00);
  }
  return std::nullopt;
}

// A key that starts a dead-key sequence leaves a dead state and types nothing
// yet, so it is not mapped: its character takes the text-only key instead.
LayoutMap buildMap(TISInputSourceRef source, std::string sourceId) {
  LayoutMap map{std::move(sourceId), {}};
  const auto data = static_cast<CFDataRef>(
      TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData)
  );
  if (data == nullptr) return map;
  const auto* layout =
      reinterpret_cast<const UCKeyboardLayout*>(CFDataGetBytePtr(data));
  const UInt32 keyboardType = LMGetKbdType();

  for (const Layer& layer : kLayers) {
    for (UInt16 keycode = 0; keycode < kKeycodeCount; ++keycode) {
      UInt32 deadState = 0;
      std::array<UniChar, 4> units{};
      UniCharCount length = 0;
      const OSStatus status = UCKeyTranslate(
          layout, keycode, kUCKeyActionDown, layer.carbonModifiers,
          keyboardType, 0, &deadState, units.size(), &length, units.data()
      );
      if (status != noErr || deadState != 0) continue;
      const auto scalar = singleScalar(units.data(), length);
      if (!scalar || !isText(*scalar)) continue;
      map.strokes.try_emplace(*scalar, KeyStroke{keycode, layer.flags});
    }
  }
  return map;
}

}  // namespace

std::optional<KeyStroke> keyStrokeForCharacter(const char32_t codepoint) {
  static std::mutex mutex;
  static LayoutMap cached;

  TISInputSourceRef source = TISCopyCurrentKeyboardLayoutInputSource();
  if (source == nullptr) return std::nullopt;
  std::string sourceId = inputSourceId(source);

  const std::scoped_lock lock(mutex);
  // The user can switch layouts between two calls, so the map follows the
  // current one.
  if (cached.sourceId != sourceId || sourceId.empty()) {
    cached = buildMap(source, std::move(sourceId));
  }
  CFRelease(source);

  const auto found = cached.strokes.find(codepoint);
  if (found == cached.strokes.end()) return std::nullopt;
  return found->second;
}

}  // namespace robot::mac

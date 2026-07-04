#include "WinKeyboardBackend.h"

#include <Windows.h>

#include <array>

#include "WinKeyMap.h"

namespace robot::win {
namespace {

std::expected<void, Error> sendScanCode(
    const ScanCode sc, const bool down
) {
  INPUT input{};
  input.type = INPUT_KEYBOARD;
  input.ki.wScan = sc.code;
  input.ki.dwFlags = KEYEVENTF_SCANCODE;
  if (!down) input.ki.dwFlags |= KEYEVENTF_KEYUP;
  if (sc.extended) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;

  if (SendInput(1, &input, sizeof(INPUT)) != 1) {
    return std::unexpected(
        Error::platformError("SendInput", static_cast<long>(GetLastError()))
    );
  }
  return {};
}

// Send one UTF-16 code unit as a Unicode character event pair component.
std::expected<void, Error> sendUnit(const WORD unit, const bool down) {
  INPUT input{};
  input.type = INPUT_KEYBOARD;
  input.ki.wScan = unit;
  input.ki.dwFlags = KEYEVENTF_UNICODE;
  if (!down) input.ki.dwFlags |= KEYEVENTF_KEYUP;

  if (SendInput(1, &input, sizeof(INPUT)) != 1) {
    return std::unexpected(
        Error::platformError("SendInput", static_cast<long>(GetLastError()))
    );
  }
  return {};
}

}  // namespace

std::expected<void, Error> WinKeyboardBackend::keyDown(const Key key) {
  const ScanCode sc = keyToScanCode(key);
  if (!sc.valid) return std::unexpected(Error::unmappableInput(toString(key)));
  return sendScanCode(sc, true);
}

std::expected<void, Error> WinKeyboardBackend::keyUp(const Key key) {
  const ScanCode sc = keyToScanCode(key);
  if (!sc.valid) return std::unexpected(Error::unmappableInput(toString(key)));
  return sendScanCode(sc, false);
}

std::expected<void, Error> WinKeyboardBackend::typeUnicode(
    const char32_t codepoint
) {
  // Encode as UTF-16. Characters above the BMP need a surrogate pair, sent as two
  // separate KEYEVENTF_UNICODE units; Windows reassembles them.
  std::array<WORD, 2> units{};
  int len = 0;
  if (codepoint <= 0xFFFF) {
    units[0] = static_cast<WORD>(codepoint);
    len = 1;
  } else {
    const char32_t v = codepoint - 0x10000;
    units[0] = static_cast<WORD>(0xD800 + (v >> 10));
    units[1] = static_cast<WORD>(0xDC00 + (v & 0x3FF));
    len = 2;
  }

  for (int i = 0; i < len; ++i) {
    if (auto r = sendUnit(units[i], true); !r) return r;
  }
  for (int i = 0; i < len; ++i) {
    if (auto r = sendUnit(units[i], false); !r) return r;
  }
  return {};
}

}  // namespace robot::win
#include "WinScreenBackend.h"

#include <shellscalingapi.h>

#include <cmath>
#include <cstdint>
#include <format>
#include <string>

namespace robot::win {
namespace {

// Per-monitor scale factor from its effective DPI (96 == 100%). Resolved
// dynamically like the awareness call so the library does not hard-require the
// Shcore import lib; falls back to 1.0 if unavailable.
double monitorScale(HMONITOR monitor) {
  using GetDpiFn = HRESULT(WINAPI*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
  static GetDpiFn getDpi = [] {
    if (HMODULE shcore = LoadLibraryW(L"Shcore.dll"); shcore != nullptr) {
      return reinterpret_cast<GetDpiFn>(
          reinterpret_cast<void*>(GetProcAddress(shcore, "GetDpiForMonitor"))
      );
    }
    return static_cast<GetDpiFn>(nullptr);
  }();

  if (getDpi == nullptr) return 1.0;
  UINT dpiX = 96;
  UINT dpiY = 96;
  if (getDpi(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY) != S_OK) return 1.0;
  return static_cast<double>(dpiX) / 96.0;
}

std::wstring toWide(const std::string& s) {
  if (s.empty()) return {};
  const int len = MultiByteToWideChar(
      CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0
  );
  std::wstring w(len, L'\0');
  MultiByteToWideChar(
      CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), w.data(), len
  );
  return w;
}

}  // namespace

BOOL CALLBACK WinScreenBackend::monitorEnumProc(
    HMONITOR monitor, HDC /*dc*/, LPRECT /*rect*/, LPARAM userData
) {
  auto* out = reinterpret_cast<std::vector<Monitor>*>(userData);

  MONITORINFOEXW info{};
  info.cbSize = sizeof(info);
  if (GetMonitorInfoW(monitor, &info) == 0) return TRUE;  // Skip; keep going.

  const double scale = monitorScale(monitor);
  const RECT& r = info.rcMonitor;  // Physical pixels under Per-Monitor-V2.
  const std::int32_t left = r.left;
  const std::int32_t top = r.top;
  const std::int32_t width = r.right - r.left;
  const std::int32_t height = r.bottom - r.top;

  Monitor m;
  m.id = static_cast<std::uint32_t>(out->size() + 1);
  m.name = std::format("{}", std::string(info.szDevice, info.szDevice + wcslen(info.szDevice) == info.szDevice ? 0 : 0));
  // szDevice is wide; format a stable ASCII-ish name without a full conversion.
  {
    std::string name;
    for (wchar_t wc : std::wstring(info.szDevice)) {
      name.push_back(wc < 128 ? static_cast<char>(wc) : '?');
    }
    m.name = name;
  }
  m.isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
  m.scaleFactor = scale;

  // The rectangle is already physical; logical bounds are the physical bounds
  // divided by this monitor's scale.
  m.physicalBounds =
      PhysicalRect{{left, top}, {width, height}};
  m.logicalBounds = LogicalRect{
      {static_cast<double>(left) / scale, static_cast<double>(top) / scale},
      {static_cast<double>(width) / scale, static_cast<double>(height) / scale}};

  out->push_back(m);
  return TRUE;
}

std::expected<std::vector<Monitor>, Error>
WinScreenBackend::enumerateMonitors() {
  std::vector<Monitor> monitors;
  EnumDisplayMonitors(
      nullptr, nullptr, &WinScreenBackend::monitorEnumProc,
      reinterpret_cast<LPARAM>(&monitors)
  );
  if (monitors.empty()) {
    return std::unexpected(Error::platformError("EnumDisplayMonitors found none"));
  }
  return monitors;
}

std::expected<Image, Error> WinScreenBackend::captureRegion(
    const PhysicalRect region
) {
  const int w = region.size.width;
  const int h = region.size.height;

  HDC screen = GetDC(nullptr);
  if (screen == nullptr) {
    return std::unexpected(Error::captureFailed("GetDC(nullptr) failed"));
  }
  HDC mem = CreateCompatibleDC(screen);
  HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
  if (mem == nullptr || bmp == nullptr) {
    if (bmp != nullptr) DeleteObject(bmp);
    if (mem != nullptr) DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    return std::unexpected(Error::captureFailed("GDI object creation failed"));
  }

  HGDIOBJ old = SelectObject(mem, bmp);
  // Source coordinates are absolute virtual-desktop pixels, so a monitor at a
  // negative origin is captured by passing its negative left/top directly.
  const BOOL ok = BitBlt(
      mem, 0, 0, w, h, screen, region.origin.x, region.origin.y, SRCCOPY
  );

  std::vector<Rgba> pixels;
  std::expected<Image, Error> result =
      std::unexpected(Error::captureFailed("BitBlt failed"));

  if (ok != 0) {
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = w;
    bi.biHeight = -h;  // Negative: top-down rows, so no manual vertical flip.
    bi.biPlanes = 1;
    bi.biBitCount = 32;  // 32bpp so rows are DWORD-aligned; channels are BGRA.
    bi.biCompression = BI_RGB;

    std::vector<std::uint8_t> raw(static_cast<std::size_t>(w) * h * 4);
    const int scanned = GetDIBits(
        mem, bmp, 0, static_cast<UINT>(h), raw.data(),
        reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS
    );
    if (scanned != 0) {
      pixels.resize(static_cast<std::size_t>(w) * h);
      for (std::size_t i = 0; i < pixels.size(); ++i) {
        const std::size_t b = i * 4;
        // GDI gives B,G,R,X; normalize to canonical straight RGBA with opaque A.
        pixels[i] = Rgba{raw[b + 2], raw[b + 1], raw[b + 0], 255};
      }
      result = Image{PhysicalSize{w, h}, std::move(pixels)};
    }
  }

  SelectObject(mem, old);
  DeleteObject(bmp);
  DeleteDC(mem);
  ReleaseDC(nullptr, screen);
  return result;
}

}  // namespace robot::win
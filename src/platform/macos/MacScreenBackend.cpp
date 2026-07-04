#include "MacScreenBackend.h"

#include <dlfcn.h>

#include <cmath>
#include <cstdint>
#include <format>

namespace robot::mac {
namespace {

// Physical pixels per logical point for a display, from its current mode: the
// ratio of pixel width to point width. 2.0 on a typical Retina panel.
double displayScale(const CGDirectDisplayID id) {
  CGDisplayModeRef mode = CGDisplayCopyDisplayMode(id);
  if (mode == nullptr) return 1.0;
  const double px = static_cast<double>(CGDisplayModeGetPixelWidth(mode));
  const double pt = static_cast<double>(CGDisplayModeGetWidth(mode));
  CGDisplayModeRelease(mode);
  if (pt <= 0.0) return 1.0;
  return px / pt;
}

// Draw a CGImage into a canonical straight-RGBA8 buffer and hand back an Image
// sized to the image's actual pixel dimensions. Rgba is four contiguous bytes in
// R,G,B,A order, which is exactly the layout a big-endian alpha-last bitmap
// context writes, so the context renders directly into the pixel vector.
std::expected<Image, Error> cgImageToImage(CGImageRef img) {
  const std::size_t w = CGImageGetWidth(img);
  const std::size_t h = CGImageGetHeight(img);
  if (w == 0 || h == 0) {
    return std::unexpected(Error::captureFailed("captured image is empty"));
  }

  std::vector<Rgba> pixels(w * h);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  const auto bitmapInfo = static_cast<CGBitmapInfo>(
      static_cast<std::uint32_t>(kCGImageAlphaPremultipliedLast) |
      static_cast<std::uint32_t>(kCGBitmapByteOrder32Big)
  );
  CGContextRef ctx = CGBitmapContextCreate(
      pixels.data(), w, h, 8, w * 4, cs, bitmapInfo
  );
  CGColorSpaceRelease(cs);
  if (ctx == nullptr) {
    return std::unexpected(Error::captureFailed("CGBitmapContextCreate failed"));
  }

  CGContextDrawImage(
      ctx, CGRectMake(0, 0, static_cast<CGFloat>(w), static_cast<CGFloat>(h)),
      img
  );
  CGContextRelease(ctx);

  return Image{PhysicalSize{static_cast<std::int32_t>(w),
                            static_cast<std::int32_t>(h)},
               std::move(pixels)};
}

std::expected<CGImageRef, Error> createDisplayImage(CGDirectDisplayID id) {
  using Fn = CGImageRef (*)(CGDirectDisplayID);
  auto fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "CGDisplayCreateImage"));
  if (fn == nullptr) {
    return std::unexpected(Error::unsupported(
        "CGDisplayCreateImage is unavailable in this macOS SDK/runtime; "
        "ScreenCaptureKit support is required"
    ));
  }

  CGImageRef image = fn(id);
  if (image == nullptr) {
    if (CGPreflightScreenCaptureAccess() == 0) {
      return std::unexpected(
          Error::permissionDenied("Screen Recording (per-display capture)")
      );
    }
    return std::unexpected(Error::captureFailed("CGDisplayCreateImage"));
  }
  return image;
}

std::expected<CGImageRef, Error> createWindowListImage(CGRect rect) {
  using Fn = CGImageRef (*)(CGRect, CGWindowListOption, CGWindowID, CGWindowImageOption);
  auto fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "CGWindowListCreateImage"));
  if (fn == nullptr) {
    return std::unexpected(Error::unsupported(
        "CGWindowListCreateImage is unavailable in this macOS SDK/runtime; "
        "ScreenCaptureKit support is required"
    ));
  }

  CGImageRef image = fn(
      rect, kCGWindowListOptionOnScreenOnly, kCGNullWindowID,
      kCGWindowImageBestResolution
  );
  if (image == nullptr) {
    if (CGPreflightScreenCaptureAccess() == 0) {
      return std::unexpected(Error::permissionDenied("Screen Recording"));
    }
    return std::unexpected(Error::captureFailed("CGWindowListCreateImage"));
  }
  return image;
}

}  // namespace

std::vector<std::pair<CGDirectDisplayID, Monitor>>
MacScreenBackend::enumerate() {
  std::uint32_t count = 0;
  CGGetActiveDisplayList(0, nullptr, &count);
  std::vector<CGDirectDisplayID> ids(count);
  CGGetActiveDisplayList(count, ids.data(), &count);

  std::vector<std::pair<CGDirectDisplayID, Monitor>> result;
  result.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const CGDirectDisplayID id = ids[i];
    const CGRect b = CGDisplayBounds(id);  // Global points, top-left origin.
    const double scale = displayScale(id);

    Monitor m;
    m.id = static_cast<std::uint32_t>(id);
    m.name = std::format("Display {}", static_cast<std::uint32_t>(id));
    m.isPrimary = CGDisplayIsMain(id) != 0;
    m.scaleFactor = scale;
    m.logicalBounds = LogicalRect{
        {b.origin.x, b.origin.y}, {b.size.width, b.size.height}};
    m.physicalBounds = PhysicalRect{
        {static_cast<std::int32_t>(std::lround(b.origin.x * scale)),
         static_cast<std::int32_t>(std::lround(b.origin.y * scale))},
        {static_cast<std::int32_t>(std::lround(b.size.width * scale)),
         static_cast<std::int32_t>(std::lround(b.size.height * scale))}};
    result.emplace_back(id, m);
  }
  return result;
}

std::expected<std::vector<Monitor>, Error>
MacScreenBackend::enumerateMonitors() {
  std::vector<Monitor> monitors;
  for (auto& [id, m] : enumerate()) monitors.push_back(m);
  if (monitors.empty()) {
    return std::unexpected(Error::platformError("no active displays"));
  }
  return monitors;
}

std::expected<Image, Error> MacScreenBackend::captureRegion(
    const PhysicalRect region
) {
  // Exact monitor match: capture that display natively so the result is correct
  // for any display density (the general path below assumes the primary scale).
  for (auto& [id, m] : enumerate()) {
    if (m.physicalBounds == region) {
      auto img = createDisplayImage(id);
      if (!img) return std::unexpected(img.error());
      auto result = cgImageToImage(*img);
      CGImageRelease(*img);
      return result;
    }
  }

  // Arbitrary virtual-desktop region. Convert the physical rect to points using
  // the primary display's scale, then capture at best (pixel) resolution. For a
  // region spanning displays of differing density the point conversion uses the
  // primary scale - a documented limitation, not a silent miscalculation.
  const double scale = displayScale(CGMainDisplayID());
  const CGRect pointRect = CGRectMake(
      region.origin.x / scale, region.origin.y / scale,
      region.size.width / scale, region.size.height / scale
  );

  auto img = createWindowListImage(pointRect);
  if (!img) return std::unexpected(img.error());
  auto result = cgImageToImage(*img);
  CGImageRelease(*img);
  return result;
}

}  // namespace robot::mac

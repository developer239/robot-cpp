#include "X11ScreenBackend.h"

#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <cstdint>
#include <cstring>
#include <format>

namespace robot::x11 {
namespace {

// Extract an 8-bit channel from a packed pixel given a mask. Server visuals
// commonly use 24/32-bit TrueColor with per-channel masks, so decoding by mask is
// correct across the usual visual layouts rather than assuming a byte order.
std::uint8_t channel(const unsigned long pixel, const unsigned long mask) {
  if (mask == 0) return 0;
  unsigned long shifted = pixel & mask;
  // Shift down to the low bits.
  int shift = 0;
  unsigned long m = mask;
  while ((m & 1) == 0 && shift < 32) {
    m >>= 1;
    shifted >>= 1;
    ++shift;
  }
  // Scale the masked field to 0..255 based on its bit width.
  int bits = 0;
  while (m & 1) {
    m >>= 1;
    ++bits;
  }
  if (bits >= 8) return static_cast<std::uint8_t>(shifted >> (bits - 8));
  return static_cast<std::uint8_t>(shifted << (8 - bits));
}

}  // namespace

std::expected<std::vector<Monitor>, Error>
X11ScreenBackend::enumerateMonitors() {
  Display* dpy = connection_->display();
  const Window root = connection_->root();

  XRRScreenResources* res = XRRGetScreenResources(dpy, root);
  if (res == nullptr) {
    return std::unexpected(Error::platformError("XRRGetScreenResources"));
  }

  RROutput primary = XRRGetOutputPrimary(dpy, root);

  std::vector<Monitor> monitors;
  for (int i = 0; i < res->ncrtc; ++i) {
    XRRCrtcInfo* crtc = XRRGetCrtcInfo(dpy, res, res->crtcs[i]);
    if (crtc == nullptr) continue;

    // A CRTC with zero mode or no outputs is disconnected; skip it.
    if (crtc->mode != 0 && crtc->noutput > 0 && crtc->width > 0 &&
        crtc->height > 0) {
      const bool isPrimary =
          crtc->noutput > 0 && crtc->outputs[0] == primary;

      Monitor m;
      m.id = static_cast<std::uint32_t>(monitors.size() + 1);
      m.name = std::format("CRTC {}", i);
      m.isPrimary = isPrimary;
      m.scaleFactor = 1.0;  // X core exposes no per-monitor logical scale.
      m.physicalBounds = PhysicalRect{
          {crtc->x, crtc->y},
          {static_cast<std::int32_t>(crtc->width),
           static_cast<std::int32_t>(crtc->height)}};
      // 1:1 with physical: one desktop pixel grid.
      m.logicalBounds = LogicalRect{
          {static_cast<double>(crtc->x), static_cast<double>(crtc->y)},
          {static_cast<double>(crtc->width),
           static_cast<double>(crtc->height)}};
      monitors.push_back(m);
    }
    XRRFreeCrtcInfo(crtc);
  }
  XRRFreeScreenResources(res);

  if (monitors.empty()) {
    return std::unexpected(Error::platformError("no active CRTCs via RandR"));
  }
  return monitors;
}

std::expected<Image, Error> X11ScreenBackend::captureRegion(
    const PhysicalRect region
) {
  Display* dpy = connection_->display();
  const int w = region.size.width;
  const int h = region.size.height;

  // AllPlanes over the root at absolute coordinates: negative origins are passed
  // through directly to capture a monitor placed left of or above the primary.
  XImage* img = XGetImage(
      dpy, connection_->root(), region.origin.x, region.origin.y,
      static_cast<unsigned int>(w), static_cast<unsigned int>(h), AllPlanes,
      ZPixmap
  );
  if (img == nullptr) {
    return std::unexpected(Error::captureFailed("XGetImage returned null"));
  }

  std::vector<Rgba> pixels(static_cast<std::size_t>(w) * h);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const unsigned long p = XGetPixel(img, x, y);
      Rgba& out = pixels[static_cast<std::size_t>(y) * w + x];
      out.r = channel(p, img->red_mask);
      out.g = channel(p, img->green_mask);
      out.b = channel(p, img->blue_mask);
      out.a = 255;
    }
  }
  XDestroyImage(img);

  return Image{PhysicalSize{w, h}, std::move(pixels)};
}

}  // namespace robot::x11
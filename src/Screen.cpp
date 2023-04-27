#include <iostream>
#include "lodepng.h"

#include "./Screen.h"

namespace Robot {

Screen::Screen() {
#if defined(__APPLE__)
  colorSpace = CGColorSpaceCreateDeviceRGB();
#endif
  Capture(0, 0, -1, -1);
}

Screen::~Screen() {
#if defined(__APPLE__)
  CGColorSpaceRelease(colorSpace);
#endif
}

Pixel Screen::GetPixelColor(int x, int y) {
  if (x < 0 || y < 0 || x >= captureWidth || y >= captureHeight) {
    return {0, 0, 0};
  }
  return pixels[y * captureWidth + x];
}

DisplaySize Screen::GetScreenSize() {
#if defined(__APPLE__)
  CGRect screenBounds = CGDisplayBounds(CGMainDisplayID());
  return {
      static_cast<int>(screenBounds.size.width),
      static_cast<int>(screenBounds.size.height)};
#elif defined(_WIN32)
  return {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
#endif
}

void Screen::Capture(int x, int y, int width, int height) {
  DisplaySize screenSize = GetScreenSize();
  captureX = x;
  captureY = y;
  captureWidth = (width == -1) ? screenSize.width : width;
  captureHeight = (height == -1) ? screenSize.height : height;

  pixels.resize(captureWidth * captureHeight);
  CaptureScreen();
}

void Screen::CaptureScreen() {
#if defined(__APPLE__)
  contextRef = CGBitmapContextCreate(
      nullptr,
      captureWidth,
      captureHeight,
      8,
      captureWidth * 4,
      colorSpace,
      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host
  );

  screenshotRef = CGDisplayCreateImageForRect(
      CGMainDisplayID(),
      CGRectMake(captureX, captureY, captureWidth, captureHeight)
  );
  CGContextDrawImage(
      contextRef,
      CGRectMake(0, 0, captureWidth, captureHeight),
      screenshotRef
  );

  size_t dataSize = captureWidth * captureHeight * 4;
  unsigned char *data =
      static_cast<unsigned char *>(CGBitmapContextGetData(contextRef));
  pixels.resize(captureWidth * captureHeight);

  for (size_t i = 0; i < dataSize; i += 4) {
    size_t pixelIndex = i / 4;
    pixels[pixelIndex].r = data[i + 2];
    pixels[pixelIndex].g = data[i + 1];
    pixels[pixelIndex].b = data[i + 0];
  }

  CGImageRelease(screenshotRef);
  CGContextRelease(contextRef);
#elif defined(_WIN32)
  hScreen = GetDC(NULL);
  hMemDC = CreateCompatibleDC(hScreen);

  hBitmap = CreateCompatibleBitmap(hScreen, captureWidth, captureHeight);
  HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

  BitBlt(
      hMemDC,
      0,
      0,
      captureWidth,
      captureHeight,
      hScreen,
      captureX,
      captureY,
      SRCCOPY
  );

  BITMAPINFOHEADER bmi = {0};
  bmi.biSize = sizeof(BITMAPINFOHEADER);
  bmi.biWidth = captureWidth;
  bmi.biHeight = -captureHeight;
  bmi.biPlanes = 1;
  bmi.biBitCount = 24;
  bmi.biCompression = BI_RGB;

  GetDIBits(
      hMemDC,
      hBitmap,
      0,
      captureHeight,
      pixels.data(),
      (BITMAPINFO *)&bmi,
      DIB_RGB_COLORS
  );

  SelectObject(hMemDC, hOldBitmap);
  DeleteObject(hBitmap);
  DeleteDC(hMemDC);
  ReleaseDC(NULL, hScreen);
#endif
}

void Screen::saveAsPNG(const std::string &filename) {
  std::vector<unsigned char> png;

  // Convert the captured pixel data to RGBA format
  std::vector<unsigned char> rgbaPixels;
  rgbaPixels.reserve(captureWidth * captureHeight * 4);
  for (const Pixel &pixel : pixels) {
#if defined(__APPLE__)
    rgbaPixels.push_back(pixel.r);
    rgbaPixels.push_back(pixel.g);
    rgbaPixels.push_back(pixel.b);
    rgbaPixels.push_back(255);
#elif defined(_WIN32)
    rgbaPixels.push_back(pixel.r);
    rgbaPixels.push_back(pixel.g);
    rgbaPixels.push_back(pixel.b);
    rgbaPixels.push_back(255);
#endif
  }

  // Encode the RGBA pixel data into a PNG and save it to a file.
  unsigned error =
      lodepng::encode(png, rgbaPixels, captureWidth, captureHeight);
  if (error) {
    std::cerr << "lodepng error " << error << ": " << lodepng_error_text(error)
              << std::endl;
    return;
  }

  lodepng::save_file(png, filename);
}

std::vector<Pixel> Screen::GetPixels() const { return pixels; }

}  // namespace Robot

#pragma once

#include <memory>
#include <tuple>
#include <vector>

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif

namespace Robot {

struct DisplaySize {
  int width;
  int height;
};

struct Pixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

class Screen {
 public:
  Screen();
  ~Screen();

  Pixel GetPixelColor(int x, int y);

  DisplaySize GetScreenSize();

  void Capture(int x = 0, int y = 0, int width = -1, int height = -1);

  std::vector<Pixel> GetPixels() const;

  void SaveAsPNG(const std::string &filename);

 private:
#if defined(__APPLE__)
  CGColorSpaceRef colorSpace;
  CGContextRef contextRef;
  CGImageRef screenshotRef;
#elif defined(_WIN32)
  HDC hScreen;
  HDC hMemDC;
  HBITMAP hBitmap;
#endif

  int captureX;
  int captureY;
  int captureWidth;
  int captureHeight;

  std::vector<Pixel> pixels;

  void CaptureScreen();
};

}  // namespace Robot

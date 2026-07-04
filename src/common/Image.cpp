#include "robot/Image.h"

#include <cstddef>
#include <string>
#include <vector>

#include "lodepng.h"  // Private dependency; never surfaced in the public header.

namespace robot {

std::expected<Rgba, Error> Image::at(
    const std::int32_t x, const std::int32_t y
) const {
  if (x < 0 || y < 0 || x >= size_.width || y >= size_.height) {
    return std::unexpected(
        Error::invalidArgument("pixel coordinate out of range")
    );
  }
  const std::size_t index =
      static_cast<std::size_t>(y) * static_cast<std::size_t>(size_.width) +
      static_cast<std::size_t>(x);
  return pixels_[index];
}

std::expected<void, Error> Image::savePng(const std::string_view path) const {
  const std::size_t expectedCount =
      static_cast<std::size_t>(size_.width) *
      static_cast<std::size_t>(size_.height);
  if (size_.width <= 0 || size_.height <= 0 ||
      pixels_.size() != expectedCount) {
    return std::unexpected(
        Error::encodeFailed("image is empty or has an inconsistent size")
    );
  }

  // Rgba is already straight (non-premultiplied) RGBA in channel order, which is
  // exactly what lodepng expects, so flattening is a direct copy.
  std::vector<unsigned char> raw;
  raw.reserve(pixels_.size() * 4);
  for (const Rgba& p : pixels_) {
    raw.push_back(p.r);
    raw.push_back(p.g);
    raw.push_back(p.b);
    raw.push_back(p.a);
  }

  std::vector<unsigned char> png;
  const unsigned encodeError = lodepng::encode(
      png, raw, static_cast<unsigned>(size_.width),
      static_cast<unsigned>(size_.height)
  );
  if (encodeError) {
    return std::unexpected(Error::encodeFailed(lodepng_error_text(encodeError)));
  }

  const unsigned saveError = lodepng::save_file(png, std::string{path});
  if (saveError) {
    return std::unexpected(Error::ioError(lodepng_error_text(saveError)));
  }
  return {};
}

}  // namespace robot
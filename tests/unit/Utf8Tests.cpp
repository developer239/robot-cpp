#include <gtest/gtest.h>

#include "robot/Keyboard.h"
#include "support/MockBackend.h"

// typeText must decode UTF-8 to scalar values and inject each via typeUnicode,
// rejecting malformed input rather than typing replacement characters. These
// tests pin the decoder and the reject-don't-guess contract.
namespace robot::test {
namespace {

std::vector<char32_t> typed(std::vector<RecordedCall>& log) {
  std::vector<char32_t> out;
  for (const auto& c : log) {
    if (c.kind == RecordedCall::Kind::TypeUnicode) out.push_back(c.codepoint);
  }
  return out;
}

TEST(Utf8, DecodesAsciiAndMultibyte) {
  MockPlatformBackend backend;
  Keyboard keyboard(backend.keyboard());

  // "Aé中🙂": 1-, 2-, 3-, and 4-byte UTF-8 sequences in one string.
  // Keep the input byte-explicit so MSVC does not transcode the narrow literal
  // through the active source code page.
  const std::string input = "A\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x99\x82";
  const auto result = keyboard.typeText(input);
  ASSERT_TRUE(result.has_value());

  const auto codes = typed(backend.log());
  ASSERT_EQ(codes.size(), 4u);
  EXPECT_EQ(codes[0], U'A');
  EXPECT_EQ(codes[1], U'\u00e9');
  EXPECT_EQ(codes[2], U'\u4e2d');
  EXPECT_EQ(codes[3], U'\U0001F642');
}

TEST(Utf8, RejectsTruncatedSequence) {
  MockPlatformBackend backend;
  Keyboard keyboard(backend.keyboard());

  const std::string truncated = "\xE4\xB8";  // First two bytes of a 3-byte char.
  const auto result = keyboard.typeText(truncated);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
  EXPECT_TRUE(typed(backend.log()).empty());
}

TEST(Utf8, RejectsOverlongEncoding) {
  MockPlatformBackend backend;
  Keyboard keyboard(backend.keyboard());

  const std::string overlong = "\xC0\xAF";  // Overlong '/'.
  const auto result = keyboard.typeText(overlong);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
}

}  // namespace
}  // namespace robot::test

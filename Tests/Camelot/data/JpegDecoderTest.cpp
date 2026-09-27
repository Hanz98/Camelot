/*
 * Copyright 2024 Jan Filip
 *
 * Licensed under the MIT License. You may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "Camelot/src/data/JpegDecoder.h"

namespace camelot {

namespace {

constexpr uint32_t kWidth = 64;
constexpr uint32_t kHeight = 36;
constexpr int kTolerance = 24;  // lossy 4:2:0 on a smooth gradient

// A smooth gradient: red grows with x, green with y, blue is constant.
std::vector<std::byte> pattern() {
  std::vector<std::byte> rgba;
  rgba.reserve(static_cast<size_t>(kWidth) * kHeight * 4);
  for (uint32_t y = 0; y < kHeight; ++y) {
    for (uint32_t x = 0; x < kWidth; ++x) {
      rgba.push_back(std::byte{static_cast<uint8_t>(x * 4)});
      rgba.push_back(std::byte{static_cast<uint8_t>(y * 7)});
      rgba.push_back(std::byte{128});
      rgba.push_back(std::byte{255});
    }
  }
  return rgba;
}

int channel(const std::vector<std::byte>& rgba, uint32_t x, uint32_t y,
            uint32_t c) {
  return static_cast<int>(rgba[(static_cast<size_t>(y) * kWidth + x) * 4 + c]);
}

}  // namespace

TEST(JpegDecoderTest, RoundTripsAGeneratedPattern) {
  const std::vector<std::byte> original = pattern();
  const std::vector<std::byte> jpeg =
      JpegDecoder::encodeJpegForTests(kWidth, kHeight, original, 95);
  ASSERT_FALSE(jpeg.empty());
  EXPECT_TRUE(JpegDecoder::looksLikeJpeg(jpeg));
  EXPECT_FALSE(JpegDecoder::looksLikePng(jpeg));

  const DecodedImage image = JpegDecoder::decodeJpeg(jpeg);
  EXPECT_EQ(image.width, kWidth);
  EXPECT_EQ(image.height, kHeight);
  ASSERT_EQ(image.rgba.size(), original.size());
  for (uint32_t y = 0; y < kHeight; ++y) {
    for (uint32_t x = 0; x < kWidth; ++x) {
      for (uint32_t c = 0; c < 3; ++c) {
        EXPECT_NEAR(channel(image.rgba, x, y, c), channel(original, x, y, c),
                    kTolerance)
            << "pixel " << x << "," << y << " channel " << c;
      }
      EXPECT_EQ(channel(image.rgba, x, y, 3), 255);
    }
  }
}

TEST(JpegDecoderTest, RecognisesSignatures) {
  const std::vector<std::byte> jpeg{std::byte{0xFF}, std::byte{0xD8},
                                    std::byte{0xFF}};
  EXPECT_TRUE(JpegDecoder::looksLikeJpeg(jpeg));
  const std::vector<std::byte> png{
      std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
      std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A}};
  EXPECT_TRUE(JpegDecoder::looksLikePng(png));
  EXPECT_FALSE(JpegDecoder::looksLikeJpeg(png));
  EXPECT_FALSE(JpegDecoder::looksLikeJpeg({}));
  const std::vector<std::byte> one{std::byte{0xFF}};
  EXPECT_FALSE(JpegDecoder::looksLikeJpeg(one));
}

TEST(JpegDecoderTest, RejectsPngWithAClearMessage) {
  const std::vector<std::byte> png{
      std::byte{0x89}, std::byte{0x50}, std::byte{0x4E},
      std::byte{0x47}, std::byte{0x0D}, std::byte{0x0A},
      std::byte{0x1A}, std::byte{0x0A}, std::byte{0x00}};
  try {
    (void)JpegDecoder::decodeJpeg(png);
    FAIL() << "expected an exception";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("PNG"), std::string::npos);
  }
}

TEST(JpegDecoderTest, RejectsGarbageAndTruncatedInput) {
  const std::vector<std::byte> garbage(64, std::byte{0x42});
  EXPECT_THROW((void)JpegDecoder::decodeJpeg(garbage), std::runtime_error);
  EXPECT_THROW((void)JpegDecoder::decodeJpeg({}), std::runtime_error);

  const std::vector<std::byte> jpeg =
      JpegDecoder::encodeJpegForTests(kWidth, kHeight, pattern());
  const std::vector<std::byte> truncated(jpeg.begin(), jpeg.begin() + 20);
  EXPECT_THROW((void)JpegDecoder::decodeJpeg(truncated), std::runtime_error);
}

TEST(JpegDecoderTest, EncoderChecksTheBufferSize) {
  const std::vector<std::byte> tooSmall(10);
  EXPECT_THROW((void)JpegDecoder::encodeJpegForTests(kWidth, kHeight, tooSmall),
               std::runtime_error);
}

}  // namespace camelot

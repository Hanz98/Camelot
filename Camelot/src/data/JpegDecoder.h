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

#ifndef CAMELOT_SRC_DATA_JPEGDECODER_H_
#define CAMELOT_SRC_DATA_JPEGDECODER_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace camelot {

// An image decoded to tightly packed RGBA8 rows, top row first.
struct DecodedImage {
  uint32_t width{0};
  uint32_t height{0};
  std::vector<std::byte> rgba;
};

// JPEG decoding through libjpeg-turbo. Every call creates its own TurboJPEG
// handle, so the functions are thread-safe. PNG (and anything that is not a
// JPEG) throws std::runtime_error with a clear message.
class JpegDecoder {
 public:
  static constexpr int kDefaultQuality = 85;

  // True when the bytes start with the JPEG SOI marker.
  [[nodiscard]] static bool looksLikeJpeg(std::span<const std::byte> data);
  // True when the bytes start with the PNG signature.
  [[nodiscard]] static bool looksLikePng(std::span<const std::byte> data);

  // Decodes a JPEG to RGBA8; throws std::runtime_error on failure.
  [[nodiscard]] static DecodedImage decodeJpeg(std::span<const std::byte> data);

  // Encodes tightly packed RGBA8 rows to a baseline JPEG (4:2:0). Meant for
  // tests, which round-trip a generated pattern instead of shipping an image
  // fixture.
  [[nodiscard]] static std::vector<std::byte> encodeJpegForTests(
      uint32_t width, uint32_t height, std::span<const std::byte> rgba,
      int quality = kDefaultQuality);
};

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_JPEGDECODER_H_

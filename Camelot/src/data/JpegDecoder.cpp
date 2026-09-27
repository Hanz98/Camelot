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

#include "Camelot/src/data/JpegDecoder.h"

#include <turbojpeg.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace camelot {

namespace {

constexpr size_t kRgbaBytesPerPixel = 4;
constexpr std::array<std::byte, 2> kJpegSoi{std::byte{0xFF}, std::byte{0xD8}};
constexpr std::array<std::byte, 8> kPngSignature{
    std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
    std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A}};

template <size_t N>
bool startsWith(std::span<const std::byte> data,
                const std::array<std::byte, N>& prefix) {
  if (data.size() < N) {
    return false;
  }
  return std::equal(prefix.begin(), prefix.end(), data.begin());
}

// Owns a TurboJPEG handle for the duration of one call.
struct HandleDeleter {
  void operator()(void* handle) const { tj3Destroy(handle); }
};
using Handle = std::unique_ptr<void, HandleDeleter>;

Handle makeHandle(int initType) {
  Handle handle(tj3Init(initType));
  if (!handle) {
    throw std::runtime_error("libjpeg-turbo: cannot create a handle");
  }
  return handle;
}

[[noreturn]] void fail(const Handle& handle, const char* what) {
  throw std::runtime_error(std::string("libjpeg-turbo: ") + what + ": " +
                           tj3GetErrorStr(handle.get()));
}

// TurboJPEG's API works on unsigned char.
const unsigned char* asUChar(std::span<const std::byte> data) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const unsigned char*>(data.data());
}

unsigned char* asMutableUChar(std::span<std::byte> data) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<unsigned char*>(data.data());
}

}  // namespace

bool JpegDecoder::looksLikeJpeg(std::span<const std::byte> data) {
  return startsWith(data, kJpegSoi);
}

bool JpegDecoder::looksLikePng(std::span<const std::byte> data) {
  return startsWith(data, kPngSignature);
}

DecodedImage JpegDecoder::decodeJpeg(std::span<const std::byte> data) {
  if (looksLikePng(data)) {
    throw std::runtime_error(
        "JpegDecoder: PNG images are not supported, only JPEG");
  }
  if (!looksLikeJpeg(data)) {
    throw std::runtime_error("JpegDecoder: the data is not a JPEG image");
  }
  Handle handle = makeHandle(TJINIT_DECOMPRESS);
  if (tj3DecompressHeader(handle.get(), asUChar(data), data.size()) != 0) {
    fail(handle, "invalid JPEG header");
  }
  const int width = tj3Get(handle.get(), TJPARAM_JPEGWIDTH);
  const int height = tj3Get(handle.get(), TJPARAM_JPEGHEIGHT);
  if (width <= 0 || height <= 0) {
    throw std::runtime_error("JpegDecoder: JPEG has an empty size");
  }
  DecodedImage image;
  image.width = static_cast<uint32_t>(width);
  image.height = static_cast<uint32_t>(height);
  image.rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) *
                    kRgbaBytesPerPixel);
  if (tj3Decompress8(handle.get(), asUChar(data), data.size(),
                     asMutableUChar(image.rgba), 0, TJPF_RGBA) != 0) {
    fail(handle, "decode failed");
  }
  return image;
}

std::vector<std::byte> JpegDecoder::encodeJpegForTests(
    uint32_t width, uint32_t height, std::span<const std::byte> rgba,
    int quality) {
  if (rgba.size() != static_cast<size_t>(width) * height * kRgbaBytesPerPixel) {
    throw std::runtime_error(
        "JpegDecoder: the RGBA buffer does not match width * height * 4");
  }
  Handle handle = makeHandle(TJINIT_COMPRESS);
  tj3Set(handle.get(), TJPARAM_QUALITY, quality);
  tj3Set(handle.get(), TJPARAM_SUBSAMP, TJSAMP_420);
  unsigned char* jpeg = nullptr;
  size_t jpegSize = 0;
  const int result =
      tj3Compress8(handle.get(), asUChar(rgba), static_cast<int>(width), 0,
                   static_cast<int>(height), TJPF_RGBA, &jpeg, &jpegSize);
  const std::unique_ptr<unsigned char, decltype(&tj3Free)> owned(jpeg,
                                                                 &tj3Free);
  if (result != 0) {
    fail(handle, "encode failed");
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* first = reinterpret_cast<const std::byte*>(jpeg);
  const std::span<const std::byte> bytes(first, jpegSize);
  return {bytes.begin(), bytes.end()};
}

}  // namespace camelot

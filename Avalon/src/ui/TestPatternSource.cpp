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

#include "Avalon/src/ui/TestPatternSource.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace avalon {

namespace {
constexpr uint32_t kBarHalfWidth = 6;
constexpr uint32_t kStripeHeight = 8;
}  // namespace

TestPatternSource::TestPatternSource(uint32_t width, uint32_t height,
                                     double fps, uint64_t frameLimit)
    : m_width(width), m_height(height), m_fps(fps), m_frameLimit(frameLimit) {
  if (width == 0 || height == 0 || fps <= 0.0) {
    spdlog::error("TestPatternSource: size and fps must be positive.");
    throw std::runtime_error(
        "TestPatternSource: size and fps must be positive.");
  }
}

uint32_t TestPatternSource::barX(uint64_t index) const {
  return static_cast<uint32_t>(index * 4 % m_width);
}

void TestPatternSource::render(uint64_t index,
                               std::span<std::byte> rgba) const {
  if (rgba.size() < frameBytes()) {
    spdlog::error("TestPatternSource: buffer too small ({} < {}).", rgba.size(),
                  frameBytes());
    throw std::runtime_error("TestPatternSource: buffer too small.");
  }
  const uint32_t bar = barX(index);
  const uint32_t stripeCells = std::max<uint32_t>(m_width / 16, 1);
  for (uint32_t y = 0; y < m_height; ++y) {
    for (uint32_t x = 0; x < m_width; ++x) {
      const size_t offset = (static_cast<size_t>(y) * m_width + x) * 4;
      auto r = static_cast<uint8_t>(x * 255 / m_width);
      auto g = static_cast<uint8_t>(y * 255 / m_height);
      uint8_t b = 96;
      const bool inBar = x + kBarHalfWidth >= bar && x <= bar + kBarHalfWidth;
      if (inBar) {
        r = 255;
        g = 255;
        b = 255;
      }
      if (y < kStripeHeight) {
        // Binary frame counter, one cell per bit, most significant left.
        const uint32_t cell = x / stripeCells;
        const bool on = cell < 16 && ((index >> (15 - cell)) & 1U) != 0;
        r = g = b = on ? 255 : 0;
      }
      rgba[offset] = static_cast<std::byte>(r);
      rgba[offset + 1] = static_cast<std::byte>(g);
      rgba[offset + 2] = static_cast<std::byte>(b);
      rgba[offset + 3] = static_cast<std::byte>(255);
    }
  }
}

bool TestPatternSource::nextFrame(std::span<std::byte> rgba,
                                  double& timestamp) {
  if (m_frameLimit != 0 && m_frame >= m_frameLimit) {
    return false;
  }
  render(m_frame, rgba);
  timestamp = static_cast<double>(m_frame) / m_fps;
  ++m_frame;
  return true;
}

}  // namespace avalon

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

#ifndef AVALON_SRC_UI_TESTPATTERNSOURCE_H_
#define AVALON_SRC_UI_TESTPATTERNSOURCE_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "Avalon/src/ui/VideoSource.h"

namespace avalon {

// Synthetic video: colour gradient, a bar sweeping left to right and a
// frame-counter stripe. Deterministic per frame index. Loops forever unless
// a frame limit is set.
class TestPatternSource : public IVideoSource {
 private:
  uint32_t m_width;
  uint32_t m_height;
  double m_fps;
  uint64_t m_frame{0};
  uint64_t m_frameLimit{0};  // 0 = endless

 public:
  TestPatternSource(uint32_t width, uint32_t height, double fps = 30.0,
                    uint64_t frameLimit = 0);

  [[nodiscard]] uint32_t width() const override { return m_width; }
  [[nodiscard]] uint32_t height() const override { return m_height; }
  [[nodiscard]] double framesPerSecond() const override { return m_fps; }
  [[nodiscard]] uint64_t frameIndex() const { return m_frame; }

  bool nextFrame(std::span<std::byte> rgba, double& timestamp) override;
  void rewind() override { m_frame = 0; }

  // Fills `rgba` with frame `index`; used by nextFrame() and tests.
  void render(uint64_t index, std::span<std::byte> rgba) const;
  // Horizontal centre of the sweeping bar in frame `index`, in pixels.
  [[nodiscard]] uint32_t barX(uint64_t index) const;
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_TESTPATTERNSOURCE_H_

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

#ifndef AVALON_SRC_UI_VIDEOSOURCE_H_
#define AVALON_SRC_UI_VIDEOSOURCE_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace avalon {

// Produces RGBA8 frames of a fixed size. A decoder (ffmpeg, MCAP images)
// implements this; TestPatternSource is the built-in stand-in.
class IVideoSource {
 public:
  IVideoSource() = default;
  IVideoSource(const IVideoSource&) = default;
  IVideoSource& operator=(const IVideoSource&) = default;
  IVideoSource(IVideoSource&&) = default;
  IVideoSource& operator=(IVideoSource&&) = default;
  virtual ~IVideoSource() = default;

  [[nodiscard]] virtual uint32_t width() const = 0;
  [[nodiscard]] virtual uint32_t height() const = 0;
  [[nodiscard]] virtual double framesPerSecond() const = 0;
  [[nodiscard]] size_t frameBytes() const {
    return static_cast<size_t>(width()) * height() * 4;
  }

  // Writes the next frame (width * height * 4 bytes, RGBA, row-major) and
  // its timestamp. Returns false at the end of the stream.
  virtual bool nextFrame(std::span<std::byte> rgba, double& timestamp) = 0;
  virtual void rewind() = 0;
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_VIDEOSOURCE_H_

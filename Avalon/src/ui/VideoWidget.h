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

#ifndef AVALON_SRC_UI_VIDEOWIDGET_H_
#define AVALON_SRC_UI_VIDEOWIDGET_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Avalon/src/ui/VideoSource.h"
#include "Avalon/src/ui/VideoTexture.h"

namespace avalon {

// Plays an IVideoSource into a VideoTexture at the source's frame rate and
// shows it with play/pause controls. Overlays go through texture().
class VideoWidget {
 private:
  std::string m_title;
  std::shared_ptr<IVideoSource> m_source;
  std::shared_ptr<VideoTexture> m_texture;
  std::vector<std::byte> m_frame;
  bool m_playing{true};
  bool m_ended{false};
  double m_clock{0.0};
  double m_nextFrameAt{0.0};
  double m_timestamp{0.0};
  uint64_t m_framesShown{0};

 public:
  VideoWidget(std::string title, std::shared_ptr<IVideoSource> source,
              std::shared_ptr<VideoTexture> texture);

  // Advances the clock and pulls frames that are due. Call once per frame.
  void tick(double deltaSeconds);
  // Draws the controls and the video, fitted to the available width.
  void draw();

  void play() { m_playing = true; }
  void pause() { m_playing = false; }
  void rewind();
  [[nodiscard]] bool isPlaying() const { return m_playing; }
  [[nodiscard]] bool hasEnded() const { return m_ended; }
  [[nodiscard]] uint64_t framesShown() const { return m_framesShown; }
  [[nodiscard]] double timestamp() const { return m_timestamp; }
  [[nodiscard]] const std::shared_ptr<VideoTexture>& texture() const {
    return m_texture;
  }
  [[nodiscard]] const std::shared_ptr<IVideoSource>& source() const {
    return m_source;
  }
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_VIDEOWIDGET_H_

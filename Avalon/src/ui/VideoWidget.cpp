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

#include "Avalon/src/ui/VideoWidget.h"

#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace avalon {

VideoWidget::VideoWidget(std::string title,
                         std::shared_ptr<IVideoSource> source,
                         std::shared_ptr<VideoTexture> texture)
    : m_title(std::move(title)),
      m_source(std::move(source)),
      m_texture(std::move(texture)) {
  if (m_source == nullptr || m_texture == nullptr) {
    spdlog::error("VideoWidget: source or texture is null.");
    throw std::runtime_error("VideoWidget: source or texture is null.");
  }
  if (m_source->width() != m_texture->width() ||
      m_source->height() != m_texture->height()) {
    spdlog::error("VideoWidget: source is {}x{} but the texture is {}x{}.",
                  m_source->width(), m_source->height(), m_texture->width(),
                  m_texture->height());
    throw std::runtime_error("VideoWidget: source and texture sizes differ.");
  }
  m_frame.resize(m_source->frameBytes());
}

void VideoWidget::rewind() {
  m_source->rewind();
  m_ended = false;
  m_clock = 0.0;
  m_nextFrameAt = 0.0;
  m_timestamp = 0.0;
}

void VideoWidget::tick(double deltaSeconds) {
  if (!m_playing || m_ended) {
    return;
  }
  m_clock += std::max(deltaSeconds, 0.0);
  const double frameInterval = 1.0 / m_source->framesPerSecond();
  // Pull at most a few frames per tick so a long stall does not spin.
  int pulled = 0;
  while (m_clock >= m_nextFrameAt && pulled < 4) {
    if (!m_source->nextFrame(m_frame, m_timestamp)) {
      m_ended = true;
      break;
    }
    m_texture->upload(m_frame);
    ++m_framesShown;
    m_nextFrameAt += frameInterval;
    ++pulled;
  }
  if (pulled == 4) {
    m_nextFrameAt = m_clock;  // resync after a stall
  }
}

void VideoWidget::draw() {
  if (ImGui::Button(m_playing ? "Pause" : "Play")) {
    m_playing = !m_playing;
  }
  ImGui::SameLine();
  if (ImGui::Button("Rewind")) {
    rewind();
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(fmt::format("{}  {:.2f}s  frame {}  {}x{}  {}",
                                     m_title, m_timestamp, m_framesShown,
                                     m_texture->width(), m_texture->height(),
                                     m_ended ? "(end)" : "")
                             .c_str());
  if (!m_texture->hasFrame()) {
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted("waiting for the first frame");
    ImGui::PopStyleColor();
    return;
  }
  const ImVec2 avail = ImGui::GetContentRegionAvail();
  const float aspect = static_cast<float>(m_texture->width()) /
                       static_cast<float>(m_texture->height());
  float width = std::max(avail.x, 64.0F);
  float height = width / aspect;
  if (avail.y > 64.0F && height > avail.y) {
    height = avail.y;
    width = height * aspect;
  }
  // ImGui identifies textures by an integer; the Vulkan backend expects the
  // descriptor set handle in it.
  const auto id =
      reinterpret_cast<ImTextureID>(m_texture->uiTexture());  // NOLINT
  ImGui::Image(ImTextureRef(id), ImVec2(width, height));
}

}  // namespace avalon

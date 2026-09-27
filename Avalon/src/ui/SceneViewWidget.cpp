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

#include "Avalon/src/ui/SceneViewWidget.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace avalon {

SceneViewWidget::SceneViewWidget(std::string title,
                                 std::shared_ptr<Device> device,
                                 std::shared_ptr<VmaAllocatorWrapper> allocator,
                                 const UiContext* ui, const Renderer* renderer,
                                 uint32_t framesInFlight, uint32_t width,
                                 uint32_t height)
    : m_device(std::move(device)),
      m_allocator(std::move(allocator)),
      m_ui(ui),
      m_renderer(renderer),
      m_title(std::move(title)),
      m_controller(&m_camera) {
  if (m_device == nullptr || m_allocator == nullptr || m_ui == nullptr ||
      m_renderer == nullptr) {
    spdlog::error(
        "SceneViewWidget: device, allocator, UI or renderer is null.");
    throw std::runtime_error(
        "SceneViewWidget: device, allocator, UI or renderer is null.");
  }
  m_target = std::make_unique<RenderTarget>(m_device, m_allocator, m_ui,
                                            std::max(width, kMinSize),
                                            std::max(height, kMinSize));
  m_pipelines = std::make_unique<PipelineManager>(m_device);
  m_cameraLayout = std::make_unique<DescriptorSetLayout>(
      m_device,
      std::vector<VkDescriptorSetLayoutBinding>{
          DescriptorSetLayout::uniformBuffer(
              0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)});
  m_descriptorPool = std::make_unique<DescriptorPool>(
      m_device,
      std::vector<VkDescriptorPoolSize>{
          {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           .descriptorCount = framesInFlight}},
      framesInFlight);
  m_cameraUbo = std::make_unique<UniformBuffer>(m_allocator, sizeof(CameraUbo),
                                                framesInFlight);
  m_cameraSets =
      m_descriptorPool->allocate(m_cameraLayout->get(), framesInFlight);
  for (uint32_t frame = 0; frame < framesInFlight; ++frame) {
    m_descriptorPool->writeUniformBuffer(m_cameraSets.at(frame), 0,
                                         m_cameraUbo->descriptorInfo(frame));
  }
}

void SceneViewWidget::requestSize(uint32_t width, uint32_t height) {
  m_requestedWidth = std::max(width, kMinSize);
  m_requestedHeight = std::max(height, kMinSize);
}

void SceneViewWidget::writeCamera(uint32_t frameIndex) {
  m_camera.setAspect(m_target->width(), m_target->height());
  m_cameraUbo->write(frameIndex, m_camera.ubo());
}

void SceneViewWidget::recordPrePass(VkCommandBuffer commandBuffer,
                                    uint32_t frameIndex) {
  if (m_requestedWidth != 0 && m_requestedHeight != 0) {
    m_target->resize(m_requestedWidth, m_requestedHeight);
    m_requestedWidth = 0;
    m_requestedHeight = 0;
  }
  writeCamera(frameIndex);
  m_target->beginPass(commandBuffer, m_clearColor);
  FrameContext context;
  context.commandBuffer = commandBuffer;
  context.frameIndex = frameIndex;
  context.extent = m_target->extent();
  context.renderPass = m_target->renderPass();
  context.samples = VK_SAMPLE_COUNT_1_BIT;
  context.cameraSet = m_cameraSets.at(frameIndex);
  context.cameraSetLayout = m_cameraLayout->get();
  context.pipelines = m_pipelines.get();
  for (const std::shared_ptr<IDrawable>& drawable : m_renderer->drawables()) {
    drawable->record(context);
  }
  m_target->endPass(commandBuffer);
  ++m_renderedFrames;
}

void SceneViewWidget::handleInput() {
  // The invisible button owns the mouse while a drag is in progress, so the
  // drag keeps working when the cursor leaves the window.
  const ImGuiIO& io = ImGui::GetIO();
  const ImVec2 mouse = io.MousePos;
  static constexpr std::array<int, 3> kGlfwButtons = {GLFW_MOUSE_BUTTON_LEFT,
                                                      GLFW_MOUSE_BUTTON_RIGHT,
                                                      GLFW_MOUSE_BUTTON_MIDDLE};
  for (int button = 0; button < 3; ++button) {
    if (m_hovered && ImGui::IsMouseClicked(button)) {
      m_controller.onMouseButton(kGlfwButtons.at(static_cast<size_t>(button)),
                                 GLFW_PRESS, mouse.x, mouse.y);
    }
    if (ImGui::IsMouseReleased(button) &&
        (m_controller.isOrbiting() || m_controller.isPanning())) {
      m_controller.onMouseButton(kGlfwButtons.at(static_cast<size_t>(button)),
                                 GLFW_RELEASE, mouse.x, mouse.y);
    }
  }
  if (m_controller.isOrbiting() || m_controller.isPanning()) {
    m_controller.onCursorMove(mouse.x, mouse.y);
  }
  if (m_hovered && io.MouseWheel != 0.0F) {
    m_controller.onScroll(io.MouseWheel);
  }
}

void SceneViewWidget::draw() {
  if (ImGui::SmallButton("Reset")) {
    m_camera.reset();
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(
      fmt::format("{}x{}  yaw {:.2f} pitch {:.2f} dist {:.2f}",
                  m_target->width(), m_target->height(), m_camera.getYaw(),
                  m_camera.getPitch(), m_camera.getDistance())
          .c_str());
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Text,
                        ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  ImGui::TextUnformatted("left: orbit  right/middle: pan  wheel: zoom");
  ImGui::PopStyleColor();

  const ImVec2 avail = ImGui::GetContentRegionAvail();
  const auto wantWidth = static_cast<uint32_t>(std::max(avail.x, 1.0F));
  const auto wantHeight = static_cast<uint32_t>(std::max(avail.y, 1.0F));
  if (wantWidth != m_target->width() || wantHeight != m_target->height()) {
    requestSize(wantWidth, wantHeight);
  }

  // Draw the current target stretched to the region (one frame of stretch
  // while a resize is pending is invisible in practice).
  const ImVec2 size(static_cast<float>(std::max(wantWidth, kMinSize)),
                    static_cast<float>(std::max(wantHeight, kMinSize)));
  const ImVec2 topLeft = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("##view", size,
                         ImGuiButtonFlags_MouseButtonLeft |
                             ImGuiButtonFlags_MouseButtonRight |
                             ImGuiButtonFlags_MouseButtonMiddle);
  m_hovered = ImGui::IsItemHovered();
  // ImGui identifies textures by an integer; the Vulkan backend expects the
  // descriptor set handle in it.
  const auto id =
      reinterpret_cast<ImTextureID>(m_target->uiTexture());  // NOLINT
  ImGui::GetWindowDrawList()->AddImage(
      ImTextureRef(id), topLeft,
      ImVec2(topLeft.x + size.x, topLeft.y + size.y));
  handleInput();
}

}  // namespace avalon

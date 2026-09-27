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

#ifndef AVALON_SRC_UI_UICONTEXT_H_
#define AVALON_SRC_UI_UICONTEXT_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

#include "Avalon/src/device/Device.h"
#include "Avalon/src/device/Instance.h"
#include "Avalon/src/window/Window.h"

struct ImGuiContext;
struct ImPlotContext;

namespace avalon {

struct UiInitInfo {
  std::shared_ptr<Instance> instance;
  std::shared_ptr<Device> device;
  std::shared_ptr<Window> window;
  VkRenderPass renderPass{VK_NULL_HANDLE};  // the pass the UI is drawn in
  uint32_t subpass{0};
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
  uint32_t minImageCount{2};
  uint32_t imageCount{2};
};

// Owns the Dear ImGui and ImPlot contexts and the GLFW + Vulkan backends.
// One frame is newFrame() -> build widgets -> render(commandBuffer) inside
// the render pass given at construction. Docking is enabled.
class UiContext {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Window> m_window;
  ImGuiContext* m_imgui{nullptr};
  ImPlotContext* m_implot{nullptr};
  bool m_frameOpen{false};
  uint64_t m_renderedFrames{0};

 public:
  static constexpr uint32_t kDescriptorPoolSize = 64;

  explicit UiContext(const UiInitInfo& info);
  UiContext(const UiContext&) = delete;
  UiContext& operator=(const UiContext&) = delete;
  UiContext(UiContext&&) = delete;
  UiContext& operator=(UiContext&&) = delete;
  ~UiContext();

  void cleanUp();

  // Starts a UI frame; widgets are built between this and render().
  void newFrame();
  // Finishes the frame and records its draw data. Does nothing if no frame
  // is open.
  void render(VkCommandBuffer commandBuffer);
  // Drops the open frame without drawing (e.g. the swapchain was rebuilt).
  void discardFrame();

  // A texture usable with ImGui::Image(); release it with unregisterTexture
  // before the image view goes away.
  [[nodiscard]] VkDescriptorSet registerTexture(
      VkSampler sampler, VkImageView view,
      VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;
  void unregisterTexture(VkDescriptorSet set) const;

  void setMinImageCount(uint32_t minImageCount) const;

  [[nodiscard]] bool isFrameOpen() const { return m_frameOpen; }
  [[nodiscard]] uint64_t renderedFrames() const { return m_renderedFrames; }
  // True while the mouse / keyboard is over or inside a UI element, so the
  // 3D view should ignore the input.
  [[nodiscard]] bool wantsMouse() const;
  [[nodiscard]] bool wantsKeyboard() const;
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_UICONTEXT_H_

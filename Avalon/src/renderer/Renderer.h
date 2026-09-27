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

#ifndef AVALON_SRC_RENDERER_RENDERER_H_
#define AVALON_SRC_RENDERER_RENDERER_H_

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/camera/Camera.h"
#include "Avalon/src/command/CommandPool.h"
#include "Avalon/src/data/buffers/UniformBuffer.h"
#include "Avalon/src/descriptor/DescriptorPool.h"
#include "Avalon/src/descriptor/DescriptorSetLayout.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/pipeline/PipelineManager.h"
#include "Avalon/src/presentation/renderpass/RenderPass.h"
#include "Avalon/src/presentation/swapchain/SwapchainModel.h"
#include "Avalon/src/renderer/IDrawable.h"
#include "Avalon/src/renderer/IPrePass.h"
#include "Avalon/src/sync/FrameSync.h"
#include "Avalon/src/window/Window.h"

namespace avalon {

class UiContext;

// Frame loop: acquire a swapchain image, record one command buffer that runs
// the render pass over every drawable, submit it and present. Handles
// swapchain recreation on resize / out-of-date. Owns the camera, its uniform
// buffer and descriptor set (set 0, binding 0 for every pipeline), and the
// PipelineManager.
class Renderer {
 public:
  static constexpr uint32_t kFramesInFlight = 2;
  // Name of the pipeline that draws the hard-coded clip-space triangle; kept
  // as the smallest possible smoke test of the pipeline path.
  static constexpr const char* kTrianglePipeline = "triangle";

 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<SwapchainModel> m_swapchain;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;

  std::unique_ptr<RenderPass> m_renderPass;
  std::unique_ptr<CommandPool> m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers;
  std::unique_ptr<FrameSync> m_sync;
  std::unique_ptr<PipelineManager> m_pipelines;

  Camera m_camera;
  std::unique_ptr<DescriptorSetLayout> m_cameraLayout;
  std::unique_ptr<DescriptorPool> m_descriptorPool;
  std::unique_ptr<UniformBuffer> m_cameraUbo;
  std::vector<VkDescriptorSet> m_cameraSets;

  std::vector<std::shared_ptr<IDrawable>> m_drawables;
  bool m_drawablesInMainPass{true};
  std::vector<std::shared_ptr<IPrePass>> m_prePasses;
  UiContext* m_ui{nullptr};  // not owned; drawn last inside the pass

  uint32_t m_currentFrame{0};
  uint64_t m_frameCount{0};
  std::array<float, 4> m_clearColor;

 public:
  Renderer(std::shared_ptr<Device> device, std::shared_ptr<Window> window,
           std::shared_ptr<SwapchainModel> swapchain,
           std::shared_ptr<VmaAllocatorWrapper> allocator);
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;
  ~Renderer();

  void cleanUp();

  // Renders and presents one frame. Returns false if nothing was drawn (the
  // window is minimised or the swapchain had to be recreated first).
  bool drawFrame();

  // Drawables are recorded in insertion order every frame. The renderer
  // waits for the device before dropping one so its buffers can go away.
  void addDrawable(std::shared_ptr<IDrawable> drawable);
  bool removeDrawable(const std::shared_ptr<IDrawable>& drawable);
  void clearDrawables();
  [[nodiscard]] size_t drawableCount() const { return m_drawables.size(); }
  [[nodiscard]] const std::vector<std::shared_ptr<IDrawable>>& drawables()
      const {
    return m_drawables;
  }
  // When the scene is shown through SceneViewWidgets (offscreen), the main
  // pass only clears and draws the UI.
  void setDrawablesInMainPass(bool enabled) { m_drawablesInMainPass = enabled; }
  [[nodiscard]] bool drawablesInMainPass() const {
    return m_drawablesInMainPass;
  }

  // Pre-passes record before the main render pass begins (uploads,
  // offscreen passes the main pass samples).
  void addPrePass(std::shared_ptr<IPrePass> prePass);
  bool removePrePass(const std::shared_ptr<IPrePass>& prePass);
  [[nodiscard]] size_t prePassCount() const { return m_prePasses.size(); }

  // The UI context whose draw data is rendered after the drawables. May be
  // null. The caller keeps it alive while it is set.
  void setUi(UiContext* ui) { m_ui = ui; }
  [[nodiscard]] UiContext* getUi() const { return m_ui; }

  [[nodiscard]] Camera& getCamera() { return m_camera; }
  [[nodiscard]] const Camera& getCamera() const { return m_camera; }
  [[nodiscard]] VkDescriptorSetLayout getCameraSetLayout() const {
    return m_cameraLayout->get();
  }

  void setClearColor(float r, float g, float b, float a = 1.0F);
  [[nodiscard]] uint64_t getFrameCount() const { return m_frameCount; }
  [[nodiscard]] const RenderPass& getRenderPass() const {
    return *m_renderPass;
  }
  [[nodiscard]] VkExtent2D getSwapchainExtent() const {
    return m_swapchain->getExtent();
  }
  [[nodiscard]] PipelineManager& getPipelineManager() const {
    return *m_pipelines;
  }

  // Waits for the device and rebuilds the swapchain for the current window
  // size. Returns false if the window is currently 0x0.
  bool recreateSwapchain();

 private:
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void createPipelines();
  void createCameraResources();
};

}  // namespace avalon

#endif  // AVALON_SRC_RENDERER_RENDERER_H_

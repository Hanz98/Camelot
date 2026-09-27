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

#include <Avalon/src/command/CommandPool.h>
#include <Avalon/src/device/Device.h>
#include <Avalon/src/presentation/renderpass/RenderPass.h>
#include <Avalon/src/presentation/swapchain/SwapChainModel.h>
#include <Avalon/src/sync/FrameSync.h>
#include <Avalon/src/window/Window.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

// Frame loop: acquire a swapchain image, record one command buffer that runs
// the render pass (currently: clear only), submit it and present. Handles
// swapchain recreation on resize / out-of-date.
class Renderer {
 public:
  static constexpr uint32_t kFramesInFlight = 2;

 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<SwapchainModel> m_swapchain;

  std::unique_ptr<RenderPass> m_renderPass;
  std::unique_ptr<CommandPool> m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers;
  std::unique_ptr<FrameSync> m_sync;

  uint32_t m_currentFrame{0};
  uint64_t m_frameCount{0};
  std::array<float, 4> m_clearColor;

 public:
  Renderer(std::shared_ptr<Device> device, std::shared_ptr<Window> window,
           std::shared_ptr<SwapchainModel> swapchain);
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;
  ~Renderer();

  void cleanUp();

  // Renders and presents one frame. Returns false if nothing was drawn (the
  // window is minimised or the swapchain had to be recreated first).
  bool drawFrame();

  void setClearColor(float r, float g, float b, float a = 1.0F);
  [[nodiscard]] uint64_t getFrameCount() const { return m_frameCount; }
  [[nodiscard]] const RenderPass& getRenderPass() const {
    return *m_renderPass;
  }
  [[nodiscard]] VkExtent2D getSwapchainExtent() const {
    return m_swapchain->getExtent();
  }

  // Waits for the device and rebuilds the swapchain for the current window
  // size. Returns false if the window is currently 0x0.
  bool recreateSwapchain();

 private:
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};

#endif  // AVALON_SRC_RENDERER_RENDERER_H_

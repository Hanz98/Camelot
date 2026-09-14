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

#ifndef AVALON_SRC_PRESENTATION_SWAPCHAIN_SWAPCHAINMODEL_H_
#define AVALON_SRC_PRESENTATION_SWAPCHAIN_SWAPCHAINMODEL_H_

#include <Avalon/src/allocator/VmaAllocator.h>
#include <Avalon/src/device/Device.h>
#include <Avalon/src/presentation/image/Image.h>
#include <Avalon/src/presentation/renderpass/RenderPass.h>
#include <Avalon/src/window/Window.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

// Owns the swapchain, its image views, the MSAA colour and depth targets and
// one framebuffer per swapchain image. Sized from the window's framebuffer.
class SwapchainModel {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;

  vkb::Swapchain m_swapchain;
  std::vector<VkImageView> m_imageViews;
  std::vector<VkFramebuffer> m_framebuffers;
  VkRenderPass m_framebufferRenderPass;  // pass the framebuffers were built for
  VkSampleCountFlagBits m_samples;

  Image m_depth;
  Image m_color;

 public:
  SwapchainModel(std::shared_ptr<Device> device, std::shared_ptr<Window> window,
                 std::shared_ptr<VmaAllocatorWrapper> allocator);
  SwapchainModel(const SwapchainModel&) = delete;
  SwapchainModel(SwapchainModel&&) = delete;
  SwapchainModel& operator=(const SwapchainModel&) = delete;
  SwapchainModel& operator=(SwapchainModel&&) = delete;

  ~SwapchainModel();
  void cleanUp();

  // Builds the swapchain for the current framebuffer size (throws if 0x0).
  void initialize();
  // Destroys and rebuilds the swapchain, targets and framebuffers for the
  // current framebuffer size. The caller must have waited for the device.
  void recreate();

  // (Re)creates one framebuffer per swapchain image for the given pass.
  void createFramebuffers(const RenderPass& renderPass);

  [[nodiscard]] const vkb::Swapchain& getVkbSwapchain() const {
    return m_swapchain;
  }
  [[nodiscard]] VkSwapchainKHR getSwapchain() const {
    return m_swapchain.swapchain;
  }
  [[nodiscard]] VkFormat getImageFormat() const {
    return m_swapchain.image_format;
  }
  [[nodiscard]] VkExtent2D getExtent() const { return m_swapchain.extent; }
  [[nodiscard]] uint32_t getImageCount() const {
    return static_cast<uint32_t>(m_imageViews.size());
  }
  [[nodiscard]] VkSampleCountFlagBits getSamples() const { return m_samples; }
  [[nodiscard]] VkFramebuffer getFramebuffer(uint32_t imageIndex) const {
    return m_framebuffers.at(imageIndex);
  }
  [[nodiscard]] bool hasFramebuffers() const { return !m_framebuffers.empty(); }

 private:
  void createSwapchain(VkSwapchainKHR oldSwapchain);
  void createImageViews();
  void createDepthImage();
  void createColorImage();
  void destroyFramebuffers();
  void destroyImageViews();
};

#endif  // AVALON_SRC_PRESENTATION_SWAPCHAIN_SWAPCHAINMODEL_H_

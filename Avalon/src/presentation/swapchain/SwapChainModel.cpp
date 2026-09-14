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

#include "SwapChainModel.h"

#include <Avalon/src/validation/CheckResult.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

SwapchainModel::SwapchainModel(std::shared_ptr<Device> device,
                               std::shared_ptr<Window> window,
                               std::shared_ptr<VmaAllocatorWrapper> allocator)
    : m_device(device),
      m_window(std::move(window)),
      m_allocator(allocator),
      m_swapchain(),
      m_framebufferRenderPass(VK_NULL_HANDLE),
      m_samples(VK_SAMPLE_COUNT_1_BIT),
      m_depth(device, allocator),
      m_color(std::move(device), std::move(allocator)) {
  if (m_device == nullptr || m_window == nullptr || m_allocator == nullptr) {
    spdlog::error("SwapchainModel: Device or Window is not initialized.");
    throw std::runtime_error(
        "SwapchainModel: Device or Window is not initialized.");
  }
  initialize();
}

SwapchainModel::~SwapchainModel() { cleanUp(); }

void SwapchainModel::cleanUp() {
  destroyFramebuffers();
  m_depth.cleanUp();
  m_color.cleanUp();
  destroyImageViews();
  if (m_swapchain.swapchain != VK_NULL_HANDLE) {
    vkb::destroy_swapchain(m_swapchain);
    m_swapchain = {};
  }
}

void SwapchainModel::initialize() {
  m_samples = m_device->getMaxUsableSampleCount();
  createSwapchain(VK_NULL_HANDLE);
  createImageViews();
  createDepthImage();
  createColorImage();
}

void SwapchainModel::recreate() {
  const VkRenderPass renderPass = m_framebufferRenderPass;
  destroyFramebuffers();
  m_depth.cleanUp();
  m_color.cleanUp();
  destroyImageViews();

  vkb::Swapchain old = m_swapchain;
  createSwapchain(old.swapchain);
  if (old.swapchain != VK_NULL_HANDLE) {
    vkb::destroy_swapchain(old);
  }
  createImageViews();
  createDepthImage();
  createColorImage();

  if (renderPass != VK_NULL_HANDLE) {
    m_framebufferRenderPass = renderPass;
    // Rebuild against the same pass the previous framebuffers used.
    std::vector<VkImageView> attachments;
    for (uint32_t i = 0; i < m_imageViews.size(); ++i) {
      attachments.clear();
      if (m_samples != VK_SAMPLE_COUNT_1_BIT) {
        attachments = {m_color.getImageView(), m_depth.getImageView(),
                       m_imageViews[i]};
      } else {
        attachments = {m_imageViews[i], m_depth.getImageView()};
      }
      VkFramebufferCreateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      info.renderPass = renderPass;
      info.attachmentCount = static_cast<uint32_t>(attachments.size());
      info.pAttachments = attachments.data();
      info.width = m_swapchain.extent.width;
      info.height = m_swapchain.extent.height;
      info.layers = 1;
      VkFramebuffer framebuffer = VK_NULL_HANDLE;
      VK_CHECK_RESULT(vkCreateFramebuffer(m_device->getDevice(), &info, nullptr,
                                          &framebuffer));
      m_framebuffers.push_back(framebuffer);
    }
  }
}

void SwapchainModel::createFramebuffers(const RenderPass& renderPass) {
  destroyFramebuffers();
  m_framebufferRenderPass = renderPass.getRenderPass();
  if (renderPass.getSamples() != m_samples) {
    spdlog::error("SwapchainModel: render pass sample count does not match.");
    throw std::runtime_error(
        "SwapchainModel: render pass sample count does not match.");
  }
  // Delegate to the shared path in recreate() without rebuilding the chain.
  std::vector<VkImageView> attachments;
  for (uint32_t i = 0; i < m_imageViews.size(); ++i) {
    if (renderPass.isMultisampled()) {
      attachments = {m_color.getImageView(), m_depth.getImageView(),
                     m_imageViews[i]};
    } else {
      attachments = {m_imageViews[i], m_depth.getImageView()};
    }
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = m_framebufferRenderPass;
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    info.width = m_swapchain.extent.width;
    info.height = m_swapchain.extent.height;
    info.layers = 1;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateFramebuffer(m_device->getDevice(), &info, nullptr,
                                        &framebuffer));
    m_framebuffers.push_back(framebuffer);
  }
}

void SwapchainModel::createSwapchain(VkSwapchainKHR oldSwapchain) {
  const auto [width, height] = m_window->getFramebufferSize();
  if (width == 0 || height == 0) {
    spdlog::error("SwapchainModel: framebuffer size is 0x0 (minimised?).");
    throw std::runtime_error("SwapchainModel: framebuffer size is 0x0.");
  }

  vkb::SwapchainBuilder builder{m_device->getVkbDevice()};
  builder.set_desired_extent(width, height)
      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .set_old_swapchain(oldSwapchain);
  auto swapRet = builder.build();
  if (!swapRet) {
    spdlog::error("Failed to create Vulkan swapchain. Error: " +
                  swapRet.error().message());
    throw std::runtime_error("Failed to create Vulkan swapchain.");
  }
  m_swapchain = swapRet.value();
}

void SwapchainModel::createImageViews() {
  auto viewsRet = m_swapchain.get_image_views();
  if (!viewsRet) {
    spdlog::error("Failed to create swapchain image views. Error: " +
                  viewsRet.error().message());
    throw std::runtime_error("Failed to create swapchain image views.");
  }
  m_imageViews = viewsRet.value();
}

void SwapchainModel::destroyImageViews() {
  if (!m_imageViews.empty()) {
    m_swapchain.destroy_image_views(m_imageViews);
    m_imageViews.clear();
  }
}

void SwapchainModel::destroyFramebuffers() {
  for (VkFramebuffer framebuffer : m_framebuffers) {
    vkDestroyFramebuffer(m_device->getDevice(), framebuffer, nullptr);
  }
  m_framebuffers.clear();
}

void SwapchainModel::createDepthImage() {
  Camelot::ImageCreateInfo depthInfo = {
      .width = static_cast<int>(m_swapchain.extent.width),
      .height = static_cast<int>(m_swapchain.extent.height),
      .mipLevels = 1,
      .numSample = m_samples,
      .format = m_device->getDepthFormat(),
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT};
  m_depth.createImage(depthInfo);
}

void SwapchainModel::createColorImage() {
  if (m_samples == VK_SAMPLE_COUNT_1_BIT) {
    return;  // no MSAA: the swapchain image is the colour target
  }
  Camelot::ImageCreateInfo colorInfo = {
      .width = static_cast<int>(m_swapchain.extent.width),
      .height = static_cast<int>(m_swapchain.extent.height),
      .mipLevels = 1,
      .numSample = m_samples,
      .format = m_swapchain.image_format,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT};
  m_color.createImage(colorInfo);
}

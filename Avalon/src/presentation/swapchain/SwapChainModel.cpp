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

#include <Avalon/src/utils/ResourceDescriptor.h>

#include <utility>

SwapchainModel::SwapchainModel()
    : m_resourceDescriptor(Resource::Descriptor::GetDescriptor()) {}

SwapchainModel::SwapchainModel(SwapchainModel&& other) noexcept
    : m_depth(std::move(other.m_depth)),
      m_color(std::move(other.m_color)),
      m_resourceDescriptor(std::move(other.m_resourceDescriptor)) {  //,
  //       m_swapChainImage(std::move(other.m_swapChainImage)),
  //       m_frameBuffers(std::move(other.m_frameBuffers))
  //{
}

SwapchainModel& SwapchainModel::operator=(SwapchainModel&& other) noexcept {
  if (this != &other) {
    m_depth = std::move(other.m_depth);
    m_color = std::move(other.m_color);
    m_resourceDescriptor = std::move(other.m_resourceDescriptor);
    //    m_swapChainImage = std::move(other.m_swapChainImage);
    //    m_frameBuffers = std::move(other.m_frameBuffers);
  }
  return *this;
}

SwapchainModel::~SwapchainModel() { cleanUp(); }

void SwapchainModel::cleanUp() { vkb::destroy_swapchain(m_swapchain); }

void SwapchainModel::initialize() {
  if (m_resourceDescriptor->getDevice() == nullptr ||
      m_resourceDescriptor->getWindow() == nullptr) {
    spdlog::error("SwapchainModel: Device or Window is not initialized.");
    throw std::runtime_error(
        "SwapchainModel: Device or Window is not initialized.");
  }

  vkb::SwapchainBuilder swapchainBuilder{
      m_resourceDescriptor->getDevice()->getVkbDevice()};
  auto swapRet = swapchainBuilder.build();
  if (!swapRet) {
    spdlog::error("Failed to create Vulkan swapchain. Error: " +
                  swapRet.error().message());
    throw std::runtime_error("Failed to create Vulkan swapchain.");
  }

  m_swapchain = swapRet.value();
}

void SwapchainModel::recreateSwapchain() {
  vkb::SwapchainBuilder swapchain_builder{
      m_resourceDescriptor->getDevice()->getVkbDevice()};
  auto swapRet = swapchain_builder.set_old_swapchain(m_swapchain).build();
  if (!swapRet) {
    m_swapchain.swapchain = VK_NULL_HANDLE;
    spdlog::error("Failed to recreate Vulkan swapchain. Error: " +
                  swapRet.error().message());
    throw std::runtime_error("Failed to recreate Vulkan swapchain.");
  }

  vkb::destroy_swapchain(m_swapchain);
  m_swapchain = swapRet.value();

  createDepthImage();
  createColorImage();
}

void SwapchainModel::createDepthImage() {
  Camelot::ImageCreateInfo depthInfo = {
      .width = m_resourceDescriptor->getWindow()->getWidth(),
      .height = m_resourceDescriptor->getWindow()->getHeight(),
      .mipLevels = 1,
      .numSample = m_resourceDescriptor->getDevice()->getMaxUsableSampleCount(),
      .format = m_resourceDescriptor->getDevice()->getDepthFormat(),
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT};

  m_depth.createImage(depthInfo);
}

void SwapchainModel::createColorImage() {
  Camelot::ImageCreateInfo colorInfo = {
      .width = m_resourceDescriptor->getWindow()->getWidth(),
      .height = m_resourceDescriptor->getWindow()->getHeight(),
      .mipLevels = 1,
      .numSample = m_resourceDescriptor->getDevice()->getMaxUsableSampleCount(),
      .format = m_swapchain.image_format,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT};

  m_color.createImage(colorInfo);
}

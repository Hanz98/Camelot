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
#include "Avalon/src/device/Device.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace avalon {

Device::Device(const std::shared_ptr<Instance>& instance,
               const std::shared_ptr<Surface>& surface)
    : m_device(), m_physicalDevice(), m_physicalDeviceProperties() {
  initialize(instance, surface);
}

Device::Device(Device&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_physicalDevice(std::move(other.m_physicalDevice)),
      m_graphicsQueue(other.m_graphicsQueue),
      m_presentQueue(other.m_presentQueue),
      m_graphicsQueueFamily(other.m_graphicsQueueFamily),
      m_physicalDeviceProperties(other.m_physicalDeviceProperties) {
  other.m_device = {};
  other.m_physicalDevice = {};
  other.m_graphicsQueue = VK_NULL_HANDLE;
  other.m_presentQueue = VK_NULL_HANDLE;
  other.m_physicalDeviceProperties = {};
}

Device& Device::operator=(Device&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = other.m_device;
    m_physicalDevice = other.m_physicalDevice;
    m_graphicsQueue = other.m_graphicsQueue;
    m_presentQueue = other.m_presentQueue;
    m_graphicsQueueFamily = other.m_graphicsQueueFamily;
    m_physicalDeviceProperties = other.m_physicalDeviceProperties;
    other.m_device = {};
    other.m_physicalDevice = {};
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_presentQueue = VK_NULL_HANDLE;
    other.m_physicalDeviceProperties = {};
  }
  return *this;
}

Device::~Device() { cleanUp(); }

void Device::cleanUp() {
  if (m_device) {
    vkb::destroy_device(m_device);
    m_device = {};
  }
}

void Device::initialize(const std::shared_ptr<Instance>& instance,
                        const std::shared_ptr<Surface>& surface) {
  if (surface == nullptr || instance == nullptr) {
    spdlog::error("Device::pickPhysicalDevice invalid arguments!");
    throw std::runtime_error("Device::pickPhysicalDevice invalid arguments!");
  }

  vkb::PhysicalDeviceSelector selector{instance->getVkbInstance()};
  auto vkbPhysicalDevice = selector.set_surface(surface->getSurface())
                               .set_minimum_version(1, 1)
                               .select();
  if (!vkbPhysicalDevice) {
    spdlog::error("Failed to select Vulkan Physical Device. Error: " +
                  vkbPhysicalDevice.error().message());
    for (const std::string& reason :
         vkbPhysicalDevice.detailed_failure_reasons()) {
      spdlog::error("  {}", reason);
    }
    throw std::runtime_error("Failed to select Vulkan Physical Device.");
  }
  m_physicalDevice = vkbPhysicalDevice.value();
  // Optional: memory budget queries for VMA statistics.
  m_physicalDevice.enable_extension_if_present(
      VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

  vkb::DeviceBuilder deviceBuilder{m_physicalDevice};
  auto devRet = deviceBuilder.build();
  if (!devRet) {
    spdlog::error("Failed to create Vulkan device. Error: " +
                  devRet.error().message());
    throw std::runtime_error("Failed to create Vulkan device.");
  }

  m_device = devRet.value();

  initializeQueues();
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
}

void Device::initializeQueues() {
  auto graphicsQueueRet = m_device.get_queue(vkb::QueueType::graphics);
  if (!graphicsQueueRet) {
    spdlog::error("Failed to create Vulkan queue. Error: " +
                  graphicsQueueRet.error().message());
    throw std::runtime_error("Failed to create Vulkan queue.");
  }
  m_graphicsQueue = graphicsQueueRet.value();

  auto presentQueueRet = m_device.get_queue(vkb::QueueType::present);
  if (!presentQueueRet) {
    spdlog::error("Failed to create Vulkan queue. Error: " +
                  presentQueueRet.error().message());
    throw std::runtime_error("Failed to create Vulkan queue.");
  }
  m_presentQueue = presentQueueRet.value();

  auto familyRet = m_device.get_queue_index(vkb::QueueType::graphics);
  if (!familyRet) {
    spdlog::error("Failed to query graphics queue family. Error: " +
                  familyRet.error().message());
    throw std::runtime_error("Failed to query graphics queue family.");
  }
  m_graphicsQueueFamily = familyRet.value();
}

void Device::waitIdle() const {
  if (m_device.device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(m_device.device);
  }
}

bool Device::isExtensionEnabled(const char* name) const {
  for (const auto& extension : m_physicalDevice.get_extensions()) {
    if (extension == name) {
      return true;
    }
  }
  return false;
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount() {
  VkSampleCountFlags counts =
      m_physicalDeviceProperties.limits.framebufferColorSampleCounts &
      m_physicalDeviceProperties.limits.framebufferDepthSampleCounts;

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }
  return VK_SAMPLE_COUNT_1_BIT;
}

VkFormat Device::getDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                     VkImageTiling tiling,
                                     VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

    const VkFormatFeatureFlags supported = tiling == VK_IMAGE_TILING_LINEAR
                                               ? props.linearTilingFeatures
                                               : props.optimalTilingFeatures;
    if ((supported & features) == features) {
      return format;
    }
  }

  spdlog::error("Failed to find supported format.");
  throw std::runtime_error("Failed to find supported format.");
}

}  // namespace avalon

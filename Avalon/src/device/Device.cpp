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
#include "Device.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

Device::Device()
    : m_device(),
      m_physicalDevice(),
      m_graphicsQueue(VK_NULL_HANDLE),
      m_presentQueue(VK_NULL_HANDLE),
      m_physicalDeviceProperties() {}

Device::Device(Device&& other)
    : m_device(other.m_device),
      m_physicalDevice(other.m_physicalDevice),
      m_graphicsQueue(other.m_graphicsQueue),
      m_presentQueue(other.m_presentQueue),
      m_physicalDeviceProperties(other.m_physicalDeviceProperties) {
  other.m_device = {};
  other.m_physicalDevice = {};
  other.m_graphicsQueue = VK_NULL_HANDLE;
  other.m_presentQueue = VK_NULL_HANDLE;
  other.m_physicalDeviceProperties = {};
}

Device& Device::operator=(Device&& other) noexcept {
  m_device = other.m_device;
  m_physicalDevice = other.m_physicalDevice;
  other.m_device = {};
  other.m_physicalDevice = {};
  return *this;
}

Device::~Device() { cleanUp(); }

void Device::cleanUp() {
  if (m_device) {
    vkb::destroy_device(m_device);
    m_device = {};
  }
}

void Device::initialize(std::shared_ptr<Instance> instance,
                        std::shared_ptr<Surface> surface) {
  if (surface == nullptr || instance == nullptr) {
    spdlog::error("Device::pickPhysicalDevice invaliad arguments!");
    throw std::runtime_error("Device::pickPhysicalDevice invaliad arguments!");
  }

  vkb::PhysicalDeviceSelector selector{instance->getVkbInstance()};
  auto vkbPhysicalDevice = selector.set_surface(surface->getSurface()).select();
  if (!vkbPhysicalDevice) {
    spdlog::error("Failed to select Vulkan Physical Device. Error: " +
                  vkbPhysicalDevice.error().message());
    throw std::runtime_error("Failed to select Vulkan Physical Device.");
  }
  m_physicalDevice = vkbPhysicalDevice.value();

  vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice.value()};
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

VkFormat Device::getDepthFormat() { return VK_FORMAT_UNDEFINED; }

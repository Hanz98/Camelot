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

#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <stdexcept>

Device::Device() : m_device(), m_physicalDevice() {}

Device::Device(Device&& other)
    : m_device(other.m_device), m_physicalDevice(other.m_physicalDevice) {
  other.m_device = {};
  other.m_physicalDevice = {};
}

Device& Device::operator=(Device&& other) {
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

void Device::PickPhysicalDevice(const Instance& instance,
                                const VkSurfaceKHR& surface) {
  vkb::PhysicalDeviceSelector selector{instance.getVkbInstance()};
  auto phys_ret = selector.set_surface(surface).select();
  m_physicalDevice = phys_ret.value();
  if (!phys_ret) {
    spdlog::error("Failed to select Vulkan Physical Device. Error: " +
                  phys_ret.error().message());
    throw std::runtime_error("Failed to select Vulkan Physical Device.");
  }

  vkb::DeviceBuilder device_builder{phys_ret.value()};
  auto dev_ret = device_builder.build();
  if (!dev_ret) {
    spdlog::error("Failed to create Vulkan device. Error: " +
                  dev_ret.error().message());
    throw std::runtime_error("Failed to create Vulkan device.");
  }

  m_device = dev_ret.value();
  volkLoadDevice(m_device.device);
}

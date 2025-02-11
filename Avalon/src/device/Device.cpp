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

#include <stdexcept>

Device::Device() : m_device(VK_NULL_HANDLE), m_physicalDevice(VK_NULL_HANDLE) {}

Device::Device(Device&& other)
    : m_device(other.m_device), m_physicalDevice(other.m_physicalDevice) {
  other.m_device = VK_NULL_HANDLE;
  other.m_physicalDevice = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) {
  m_device = other.m_device;
  m_physicalDevice = other.m_physicalDevice;
  other.m_device = VK_NULL_HANDLE;
  other.m_physicalDevice = VK_NULL_HANDLE;
  return *this;
}

Device::~Device() { cleanUp(); }

void Device::cleanUp() {
  if (m_device) {
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }
}

/*
VkPhysicalDevice Device::PickPhysicalDevice(VkInstance instance) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  for (const auto& device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      return device;
    }
  }
  return devices[0];
}

*/

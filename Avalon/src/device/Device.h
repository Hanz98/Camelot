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

#ifndef AVALON_SRC_DEVICE_DEVICE_H_
#define AVALON_SRC_DEVICE_DEVICE_H_

#include <pch.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

class Device {
 private:
  VkDevice m_device;
  VkPhysicalDevice m_physicalDevice;

  VmaAllocation m_allocation;

 public:
  Device();
  Device(Device&& other);
  Device(const Device& other) = delete;
  Device& operator=(Device&& other);
  Device& operator=(const Device& other) = delete;

  ~Device();

  void cleanUp();

 public:
  VkDevice& getDevice() { return m_device; }
  VkPhysicalDevice& getPhysicalDevice() { return m_physicalDevice; }

  /*
  static VkPhysicalDevice PickPhysicalDevice(VkInstance instance,
  VkSurfaceKHR& surface,
  VkPhysicalDevice& physicalDevice);
  */
};

#endif  // AVALON_SRC_DEVICE_DEVICE_H_

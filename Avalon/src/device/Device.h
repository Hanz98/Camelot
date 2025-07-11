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

#include <VkBootstrap.h>
#include <VkBootstrapDispatch.h>
#include <pch.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "Instance.h"
#include "IModel.h"

namespace Device {

class Model : public IModel {
 private:
  vkb::Device m_device;
  vkb::PhysicalDevice m_physicalDevice;

  VmaAllocation m_allocation;

 public:
  Model();
  Model(Model&& other);
  Model(const Model& other) = delete;
  Model& operator=(Model&& other);
  Model& operator=(const Model& other) = delete;

  ~Model() override;

  void cleanUp() override;

 public:
  VkDevice& getDevice() override { return m_device.device; }
  VkPhysicalDevice& getPhysicalDevice() override {
    return m_physicalDevice.physical_device;
  }

  void PickPhysicalDevice(const Instance& instance,
                          const VkSurfaceKHR& surface) override;
};

}  // namespace Device

// Legacy alias for backward compatibility - place outside namespace
class Device : public ::Device::Model {
public:
  // Inherit all constructors and methods
  using ::Device::Model::Model;
};

#endif  // AVALON_SRC_DEVICE_DEVICE_H_

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

#ifndef AVALON_SRC_DESCRIPTOR_DESCRIPTORPOOL_H_
#define AVALON_SRC_DESCRIPTOR_DESCRIPTORPOOL_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "Avalon/src/device/Device.h"

namespace avalon {

// RAII VkDescriptorPool. Sets allocated from it are freed with the pool (or
// all at once with reset()).
class DescriptorPool {
 private:
  std::shared_ptr<Device> m_device;
  VkDescriptorPool m_pool{VK_NULL_HANDLE};

 public:
  DescriptorPool(std::shared_ptr<Device> device,
                 const std::vector<VkDescriptorPoolSize>& sizes,
                 uint32_t maxSets);
  DescriptorPool(const DescriptorPool&) = delete;
  DescriptorPool& operator=(const DescriptorPool&) = delete;
  DescriptorPool(DescriptorPool&& other) noexcept;
  DescriptorPool& operator=(DescriptorPool&& other) noexcept;
  ~DescriptorPool();

  void cleanUp();
  void reset();

  // Allocates `count` sets with the same layout.
  [[nodiscard]] std::vector<VkDescriptorSet> allocate(
      VkDescriptorSetLayout layout, uint32_t count) const;

  // Points binding `binding` of `set` at a uniform buffer range.
  void writeUniformBuffer(VkDescriptorSet set, uint32_t binding,
                          const VkDescriptorBufferInfo& info) const;

  [[nodiscard]] VkDescriptorPool get() const { return m_pool; }
};

}  // namespace avalon

#endif  // AVALON_SRC_DESCRIPTOR_DESCRIPTORPOOL_H_

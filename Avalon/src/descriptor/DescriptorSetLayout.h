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

#ifndef AVALON_SRC_DESCRIPTOR_DESCRIPTORSETLAYOUT_H_
#define AVALON_SRC_DESCRIPTOR_DESCRIPTORSETLAYOUT_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "Avalon/src/device/Device.h"

namespace avalon {

// RAII VkDescriptorSetLayout built from a list of bindings.
class DescriptorSetLayout {
 private:
  std::shared_ptr<Device> m_device;
  VkDescriptorSetLayout m_layout{VK_NULL_HANDLE};
  std::vector<VkDescriptorSetLayoutBinding> m_bindings;

 public:
  DescriptorSetLayout(std::shared_ptr<Device> device,
                      std::vector<VkDescriptorSetLayoutBinding> bindings);
  DescriptorSetLayout(const DescriptorSetLayout&) = delete;
  DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
  DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
  DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;
  ~DescriptorSetLayout();

  void cleanUp();

  // A single uniform buffer visible to the given stages.
  static VkDescriptorSetLayoutBinding uniformBuffer(uint32_t binding,
                                                    VkShaderStageFlags stages,
                                                    uint32_t count = 1);
  // A combined image sampler visible to the given stages.
  static VkDescriptorSetLayoutBinding combinedImageSampler(
      uint32_t binding, VkShaderStageFlags stages, uint32_t count = 1);

  [[nodiscard]] VkDescriptorSetLayout get() const { return m_layout; }
  [[nodiscard]] const std::vector<VkDescriptorSetLayoutBinding>& bindings()
      const {
    return m_bindings;
  }
};

}  // namespace avalon

#endif  // AVALON_SRC_DESCRIPTOR_DESCRIPTORSETLAYOUT_H_

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

#include "Avalon/src/descriptor/DescriptorSetLayout.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

DescriptorSetLayout::DescriptorSetLayout(
    std::shared_ptr<Device> device,
    std::vector<VkDescriptorSetLayoutBinding> bindings)
    : m_device(std::move(device)), m_bindings(std::move(bindings)) {
  if (m_device == nullptr) {
    spdlog::error("DescriptorSetLayout: device is not initialized.");
    throw std::runtime_error("DescriptorSetLayout: device is not initialized.");
  }
  VkDescriptorSetLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = static_cast<uint32_t>(m_bindings.size());
  info.pBindings = m_bindings.data();
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device->getDevice(), &info,
                                              nullptr, &m_layout));
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_layout(std::exchange(other.m_layout, VK_NULL_HANDLE)),
      m_bindings(std::move(other.m_bindings)) {}

DescriptorSetLayout& DescriptorSetLayout::operator=(
    DescriptorSetLayout&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = std::move(other.m_device);
    m_layout = std::exchange(other.m_layout, VK_NULL_HANDLE);
    m_bindings = std::move(other.m_bindings);
  }
  return *this;
}

DescriptorSetLayout::~DescriptorSetLayout() { cleanUp(); }

void DescriptorSetLayout::cleanUp() {
  if (m_layout != VK_NULL_HANDLE && m_device != nullptr) {
    vkDestroyDescriptorSetLayout(m_device->getDevice(), m_layout, nullptr);
  }
  m_layout = VK_NULL_HANDLE;
}

VkDescriptorSetLayoutBinding DescriptorSetLayout::uniformBuffer(
    uint32_t binding, VkShaderStageFlags stages, uint32_t count) {
  return {.binding = binding,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = count,
          .stageFlags = stages,
          .pImmutableSamplers = nullptr};
}

VkDescriptorSetLayoutBinding DescriptorSetLayout::combinedImageSampler(
    uint32_t binding, VkShaderStageFlags stages, uint32_t count) {
  return {.binding = binding,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = count,
          .stageFlags = stages,
          .pImmutableSamplers = nullptr};
}

}  // namespace avalon

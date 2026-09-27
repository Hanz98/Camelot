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

#include "Avalon/src/descriptor/DescriptorPool.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

DescriptorPool::DescriptorPool(std::shared_ptr<Device> device,
                               const std::vector<VkDescriptorPoolSize>& sizes,
                               uint32_t maxSets)
    : m_device(std::move(device)) {
  if (m_device == nullptr) {
    spdlog::error("DescriptorPool: device is not initialized.");
    throw std::runtime_error("DescriptorPool: device is not initialized.");
  }
  VkDescriptorPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.maxSets = maxSets;
  info.poolSizeCount = static_cast<uint32_t>(sizes.size());
  info.pPoolSizes = sizes.data();
  VK_CHECK_RESULT(
      vkCreateDescriptorPool(m_device->getDevice(), &info, nullptr, &m_pool));
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_pool(std::exchange(other.m_pool, VK_NULL_HANDLE)) {}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = std::move(other.m_device);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
  }
  return *this;
}

DescriptorPool::~DescriptorPool() { cleanUp(); }

void DescriptorPool::cleanUp() {
  if (m_pool != VK_NULL_HANDLE && m_device != nullptr) {
    vkDestroyDescriptorPool(m_device->getDevice(), m_pool, nullptr);
  }
  m_pool = VK_NULL_HANDLE;
}

void DescriptorPool::reset() {
  if (m_pool != VK_NULL_HANDLE) {
    VK_CHECK_RESULT(vkResetDescriptorPool(m_device->getDevice(), m_pool, 0));
  }
}

std::vector<VkDescriptorSet> DescriptorPool::allocate(
    VkDescriptorSetLayout layout, uint32_t count) const {
  const std::vector<VkDescriptorSetLayout> layouts(count, layout);
  VkDescriptorSetAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool = m_pool;
  info.descriptorSetCount = count;
  info.pSetLayouts = layouts.data();
  std::vector<VkDescriptorSet> sets(count, VK_NULL_HANDLE);
  VK_CHECK_RESULT(
      vkAllocateDescriptorSets(m_device->getDevice(), &info, sets.data()));
  return sets;
}

void DescriptorPool::writeUniformBuffer(
    VkDescriptorSet set, uint32_t binding,
    const VkDescriptorBufferInfo& info) const {
  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = set;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.descriptorCount = 1;
  write.pBufferInfo = &info;
  vkUpdateDescriptorSets(m_device->getDevice(), 1, &write, 0, nullptr);
}

}  // namespace avalon

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

#include "CommandPool.h"

#include <Avalon/src/validation/CheckResult.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

CommandPool::CommandPool(std::shared_ptr<Device> device)
    : m_device(std::move(device)), m_pool(VK_NULL_HANDLE) {
  if (m_device == nullptr) {
    spdlog::error("CommandPool: Device is not initialized.");
    throw std::runtime_error("CommandPool: Device is not initialized.");
  }
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  info.queueFamilyIndex = m_device->getGraphicsQueueFamily();
  VK_CHECK_RESULT(
      vkCreateCommandPool(m_device->getDevice(), &info, nullptr, &m_pool));
}

CommandPool::~CommandPool() { cleanUp(); }

void CommandPool::cleanUp() {
  if (m_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(m_device->getDevice(), m_pool, nullptr);
    m_pool = VK_NULL_HANDLE;
  }
}

std::vector<VkCommandBuffer> CommandPool::allocate(uint32_t count) const {
  std::vector<VkCommandBuffer> buffers(count, VK_NULL_HANDLE);
  if (count == 0) {
    return buffers;
  }
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = m_pool;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = count;
  VK_CHECK_RESULT(
      vkAllocateCommandBuffers(m_device->getDevice(), &info, buffers.data()));
  return buffers;
}

void CommandPool::free(const std::vector<VkCommandBuffer>& buffers) const {
  if (buffers.empty() || m_pool == VK_NULL_HANDLE) {
    return;
  }
  vkFreeCommandBuffers(m_device->getDevice(), m_pool,
                       static_cast<uint32_t>(buffers.size()), buffers.data());
}

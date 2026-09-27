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

#include "Avalon/src/data/buffers/Buffer.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <utility>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

Buffer::Buffer() : m_allocator() {}

Buffer::~Buffer() { cleanUp(); }

void Buffer::cleanUp() {
  if (m_buffer == VK_NULL_HANDLE) {
    return;
  }
  if (m_allocator == nullptr) {
    // Destructors must not throw; log and leak rather than terminate.
    spdlog::error("Buffer: cannot destroy buffer without a valid allocator!");
    return;
  }
  vmaDestroyBuffer(m_allocator->allocator, m_buffer, m_allocation);
  m_buffer = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
}

void Buffer::createBuffer(std::shared_ptr<VmaAllocatorWrapper> allocator) {
  if (allocator == nullptr) {
    spdlog::error("Buffer: allocator is not initialized.");
    throw std::runtime_error("Buffer: allocator is not initialized.");
  }
  cleanUp();
  m_allocator = std::move(allocator);

  constexpr VkDeviceSize kDefaultBufferSize =
      static_cast<VkDeviceSize>(64) * 1024;
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = kDefaultBufferSize;
  bufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  VK_CHECK_RESULT(vmaCreateBuffer(m_allocator->allocator, &bufferInfo,
                                  &allocInfo, &m_buffer, &m_allocation,
                                  nullptr));
}

}  // namespace avalon

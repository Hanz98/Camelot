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

#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

Buffer::Buffer(std::shared_ptr<VmaAllocatorWrapper> allocator,
               VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible)
    : m_allocator(std::move(allocator)), m_size(size), m_usage(usage) {
  if (m_allocator == nullptr || m_allocator->allocator == VK_NULL_HANDLE) {
    spdlog::error("Buffer: allocator is not initialized.");
    throw std::runtime_error("Buffer: allocator is not initialized.");
  }
  if (size == 0) {
    spdlog::error("Buffer: size must be greater than zero.");
    throw std::runtime_error("Buffer: size must be greater than zero.");
  }
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  if (hostVisible) {
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }
  VmaAllocationInfo info = {};
  VK_CHECK_RESULT(vmaCreateBuffer(m_allocator->allocator, &bufferInfo,
                                  &allocInfo, &m_buffer, &m_allocation, &info));
  m_mapped = hostVisible ? info.pMappedData : nullptr;
  if (hostVisible && m_mapped == nullptr) {
    cleanUp();
    spdlog::error("Buffer: host-visible allocation was not mapped.");
    throw std::runtime_error("Buffer: host-visible allocation was not mapped.");
  }
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_allocator(std::move(other.m_allocator)),
      m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_size(std::exchange(other.m_size, 0)),
      m_usage(std::exchange(other.m_usage, 0)),
      m_mapped(std::exchange(other.m_mapped, nullptr)) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_allocator = std::move(other.m_allocator);
    m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
    m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
    m_size = std::exchange(other.m_size, 0);
    m_usage = std::exchange(other.m_usage, 0);
    m_mapped = std::exchange(other.m_mapped, nullptr);
  }
  return *this;
}

Buffer::~Buffer() { cleanUp(); }

void Buffer::cleanUp() {
  if (m_buffer != VK_NULL_HANDLE) {
    if (m_allocator == nullptr || m_allocator->allocator == VK_NULL_HANDLE) {
      // Destructors must not throw; log and leak rather than terminate.
      spdlog::error("Buffer: cannot destroy buffer without a valid allocator!");
    } else {
      vmaDestroyBuffer(m_allocator->allocator, m_buffer, m_allocation);
    }
  }
  m_buffer = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
  m_mapped = nullptr;
  m_size = 0;
}

void Buffer::write(std::span<const std::byte> bytes, VkDeviceSize offset) {
  if (!isHostVisible()) {
    spdlog::error("Buffer: write() on a buffer that is not host-visible.");
    throw std::runtime_error("Buffer: buffer is not host-visible.");
  }
  if (offset > m_size || bytes.size() > m_size - offset) {
    spdlog::error("Buffer: write of {} bytes at {} exceeds size {}.",
                  bytes.size(), offset, m_size);
    throw std::runtime_error("Buffer: write exceeds the buffer size.");
  }
  if (bytes.empty()) {
    return;
  }
  std::span<std::byte> target = mapped().subspan(offset, bytes.size());
  std::memcpy(target.data(), bytes.data(), bytes.size());
  // No-op on coherent memory; required for non-coherent host memory.
  VK_CHECK_RESULT(vmaFlushAllocation(m_allocator->allocator, m_allocation,
                                     offset, bytes.size()));
}

std::span<std::byte> Buffer::mapped() const {
  if (m_mapped == nullptr) {
    return {};
  }
  return {static_cast<std::byte*>(m_mapped), static_cast<size_t>(m_size)};
}

}  // namespace avalon

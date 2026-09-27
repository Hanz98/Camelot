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

#ifndef AVALON_SRC_DATA_BUFFERS_BUFFER_H_
#define AVALON_SRC_DATA_BUFFERS_BUFFER_H_

#include <vulkan/vulkan.h>

#include <cstddef>
#include <memory>
#include <span>

#include "Avalon/src/allocator/VmaAllocator.h"

namespace avalon {

// A VkBuffer with its VMA allocation. Host-visible buffers stay persistently
// mapped and are written with write(); device-local buffers (hostVisible ==
// false) can only be filled through a transfer, which T5 adds.
class Buffer {
 private:
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  VkBuffer m_buffer{VK_NULL_HANDLE};
  VmaAllocation m_allocation{VK_NULL_HANDLE};
  VkDeviceSize m_size{0};
  VkBufferUsageFlags m_usage{0};
  void* m_mapped{nullptr};

 public:
  Buffer() = default;
  // Throws std::runtime_error if `allocator` is null, `size` is 0 or the
  // allocation fails.
  Buffer(std::shared_ptr<VmaAllocatorWrapper> allocator, VkDeviceSize size,
         VkBufferUsageFlags usage, bool hostVisible = true);
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;
  ~Buffer();

  void cleanUp();

  // Copies `bytes` into the buffer at `offset`. Throws std::runtime_error if
  // the buffer is not host-visible or the range does not fit.
  void write(std::span<const std::byte> bytes, VkDeviceSize offset = 0);
  template <typename T>
  void write(std::span<const T> items, VkDeviceSize offset = 0) {
    write(std::span<const std::byte>(std::as_bytes(items)), offset);
  }

  [[nodiscard]] VkBuffer get() const { return m_buffer; }
  [[nodiscard]] VkDeviceSize size() const { return m_size; }
  [[nodiscard]] VkBufferUsageFlags usage() const { return m_usage; }
  [[nodiscard]] bool isValid() const { return m_buffer != VK_NULL_HANDLE; }
  [[nodiscard]] bool isHostVisible() const { return m_mapped != nullptr; }
  // The persistently mapped memory of a host-visible buffer, else empty.
  [[nodiscard]] std::span<std::byte> mapped() const;
};

}  // namespace avalon

#endif  // AVALON_SRC_DATA_BUFFERS_BUFFER_H_

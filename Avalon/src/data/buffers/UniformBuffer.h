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

#ifndef AVALON_SRC_DATA_BUFFERS_UNIFORMBUFFER_H_
#define AVALON_SRC_DATA_BUFFERS_UNIFORMBUFFER_H_

#include <vulkan/vulkan.h>

#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/data/buffers/Buffer.h"

namespace avalon {

// One host-visible uniform buffer per frame in flight, so the CPU can write
// frame N+1 while the GPU still reads frame N.
class UniformBuffer {
 private:
  std::vector<Buffer> m_buffers;
  VkDeviceSize m_size{0};

 public:
  UniformBuffer(const std::shared_ptr<VmaAllocatorWrapper>& allocator,
                VkDeviceSize size, uint32_t framesInFlight);

  void write(uint32_t frame, std::span<const std::byte> bytes);
  // Writes one trivially copyable value (typically a std140 struct). The
  // explicit dynamic-extent span keeps this from re-selecting itself.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void write(uint32_t frame, const T& value) {
    write(frame, std::span<const std::byte>(
                     std::as_bytes(std::span<const T, 1>(&value, 1))));
  }

  [[nodiscard]] VkBuffer buffer(uint32_t frame) const {
    return m_buffers.at(frame).get();
  }
  [[nodiscard]] VkDeviceSize size() const { return m_size; }
  [[nodiscard]] uint32_t frames() const {
    return static_cast<uint32_t>(m_buffers.size());
  }
  [[nodiscard]] VkDescriptorBufferInfo descriptorInfo(uint32_t frame) const;
};

}  // namespace avalon

#endif  // AVALON_SRC_DATA_BUFFERS_UNIFORMBUFFER_H_

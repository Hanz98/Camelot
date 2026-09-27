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

#include "Avalon/src/data/buffers/UniformBuffer.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>

namespace avalon {

UniformBuffer::UniformBuffer(
    const std::shared_ptr<VmaAllocatorWrapper>& allocator, VkDeviceSize size,
    uint32_t framesInFlight)
    : m_size(size) {
  if (framesInFlight == 0) {
    spdlog::error("UniformBuffer: framesInFlight must be at least 1.");
    throw std::runtime_error("UniformBuffer: framesInFlight must be >= 1.");
  }
  m_buffers.reserve(framesInFlight);
  for (uint32_t i = 0; i < framesInFlight; ++i) {
    m_buffers.emplace_back(allocator, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                           true);
  }
}

void UniformBuffer::write(uint32_t frame, std::span<const std::byte> bytes) {
  m_buffers.at(frame).write(bytes);
}

VkDescriptorBufferInfo UniformBuffer::descriptorInfo(uint32_t frame) const {
  return {.buffer = buffer(frame), .offset = 0, .range = m_size};
}

}  // namespace avalon

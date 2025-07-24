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

#include "Buffer.h"

Buffer::Buffer() : m_buffer(VK_NULL_HANDLE) {}

Buffer::~Buffer() {
  vmaDestroyBuffer(m_allocator->allocator, m_buffer, m_allocation);
}

void Buffer::createBuffer() {
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = 65536;
  bufferInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  VkBuffer buffer;
  vmaCreateBuffer(m_allocator->allocator, &bufferInfo, &allocInfo, &m_buffer,
                  &m_allocation, nullptr);
}

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

#include "Avalon/src/geometry/Vertex.h"

#include <cstddef>

namespace avalon {

VkVertexInputBindingDescription Vertex::bindingDescription(uint32_t binding) {
  return {.binding = binding,
          .stride = sizeof(Vertex),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::attributeDescriptions(
    uint32_t binding) {
  return {{{.location = 0,
            .binding = binding,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, position)},
           {.location = 1,
            .binding = binding,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, normal)},
           {.location = 2,
            .binding = binding,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(Vertex, color)}}};
}

VkVertexInputBindingDescription PointVertex::bindingDescription(
    uint32_t binding) {
  return {.binding = binding,
          .stride = sizeof(PointVertex),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
}

std::array<VkVertexInputAttributeDescription, 2>
PointVertex::attributeDescriptions(uint32_t binding) {
  return {{{.location = 0,
            .binding = binding,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(PointVertex, position)},
           {.location = 1,
            .binding = binding,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(PointVertex, color)}}};
}

}  // namespace avalon

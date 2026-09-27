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

#ifndef AVALON_SRC_GEOMETRY_VERTEX_H_
#define AVALON_SRC_GEOMETRY_VERTEX_H_

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace avalon {

// Vertex layout of the lit mesh pipeline (mesh.vert): position, normal and
// an RGBA colour, all as floats.
struct Vertex {
  glm::vec3 position{0.0F};
  glm::vec3 normal{0.0F, 0.0F, 1.0F};
  glm::vec4 color{1.0F};

  static VkVertexInputBindingDescription bindingDescription(
      uint32_t binding = 0);
  static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions(
      uint32_t binding = 0);
};

// CPU-side triangle mesh: indexed triangle list.
struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  [[nodiscard]] bool empty() const { return indices.empty(); }
  [[nodiscard]] uint32_t triangleCount() const {
    return static_cast<uint32_t>(indices.size() / 3);
  }
};

}  // namespace avalon

#endif  // AVALON_SRC_GEOMETRY_VERTEX_H_

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

#ifndef AVALON_SRC_GEOMETRY_SHAPES_H_
#define AVALON_SRC_GEOMETRY_SHAPES_H_

#include <cstdint>

#include <glm/glm.hpp>

#include "Avalon/src/geometry/Vertex.h"

namespace avalon::shapes {

// Axis-aligned box centred on the origin with per-face normals
// (24 vertices, 36 indices).
MeshData box(const glm::vec3& size = glm::vec3(1.0F),
             const glm::vec4& color = glm::vec4(1.0F));

// UV sphere centred on the origin. `segments` around Z, `rings` from pole to
// pole; (segments + 1) * (rings + 1) vertices.
MeshData sphere(float radius = 0.5F, uint32_t segments = 32,
                uint32_t rings = 16, const glm::vec4& color = glm::vec4(1.0F));

// Cylinder along +Z centred on the origin, closed at both ends.
MeshData cylinder(float radius = 0.5F, float height = 1.0F,
                  uint32_t segments = 32,
                  const glm::vec4& color = glm::vec4(1.0F));

// Appends `other` to `mesh`, offsetting its indices.
void append(MeshData& mesh, const MeshData& other);

// Applies `transform` to positions and normals in place.
void applyTransform(MeshData& mesh, const glm::mat4& transform);

}  // namespace avalon::shapes

#endif  // AVALON_SRC_GEOMETRY_SHAPES_H_

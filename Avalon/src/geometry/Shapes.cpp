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

#include "Avalon/src/geometry/Shapes.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

#include <glm/glm.hpp>

namespace avalon::shapes {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kTwoPi = 2.0F * kPi;

void addQuad(MeshData& mesh, const std::array<glm::vec3, 4>& corners,
             const glm::vec3& normal, const glm::vec4& color) {
  const auto base = static_cast<uint32_t>(mesh.vertices.size());
  for (const glm::vec3& corner : corners) {
    mesh.vertices.push_back(
        {.position = corner, .normal = normal, .color = color});
  }
  mesh.indices.insert(mesh.indices.end(),
                      {base, base + 1, base + 2, base, base + 2, base + 3});
}
}  // namespace

MeshData box(const glm::vec3& size, const glm::vec4& color) {
  const glm::vec3 h = size * 0.5F;
  MeshData mesh;
  mesh.vertices.reserve(24);
  mesh.indices.reserve(36);
  // Corners are listed counter-clockwise when seen from outside.
  addQuad(mesh,
          {{{h.x, -h.y, -h.z},
            {h.x, h.y, -h.z},
            {h.x, h.y, h.z},
            {h.x, -h.y, h.z}}},
          {1, 0, 0}, color);
  addQuad(mesh,
          {{{-h.x, h.y, -h.z},
            {-h.x, -h.y, -h.z},
            {-h.x, -h.y, h.z},
            {-h.x, h.y, h.z}}},
          {-1, 0, 0}, color);
  addQuad(mesh,
          {{{h.x, h.y, -h.z},
            {-h.x, h.y, -h.z},
            {-h.x, h.y, h.z},
            {h.x, h.y, h.z}}},
          {0, 1, 0}, color);
  addQuad(mesh,
          {{{-h.x, -h.y, -h.z},
            {h.x, -h.y, -h.z},
            {h.x, -h.y, h.z},
            {-h.x, -h.y, h.z}}},
          {0, -1, 0}, color);
  addQuad(mesh,
          {{{-h.x, -h.y, h.z},
            {h.x, -h.y, h.z},
            {h.x, h.y, h.z},
            {-h.x, h.y, h.z}}},
          {0, 0, 1}, color);
  addQuad(mesh,
          {{{-h.x, h.y, -h.z},
            {h.x, h.y, -h.z},
            {h.x, -h.y, -h.z},
            {-h.x, -h.y, -h.z}}},
          {0, 0, -1}, color);
  return mesh;
}

MeshData sphere(float radius, uint32_t segments, uint32_t rings,
                const glm::vec4& color) {
  MeshData mesh;
  segments = segments < 3 ? 3 : segments;
  rings = rings < 2 ? 2 : rings;
  mesh.vertices.reserve(static_cast<size_t>(segments + 1) * (rings + 1));
  for (uint32_t ring = 0; ring <= rings; ++ring) {
    const float v = static_cast<float>(ring) / static_cast<float>(rings);
    const float phi = v * kPi;  // 0 at the +Z pole
    const float z = std::cos(phi);
    const float r = std::sin(phi);
    for (uint32_t seg = 0; seg <= segments; ++seg) {
      const float u = static_cast<float>(seg) / static_cast<float>(segments);
      const float theta = u * kTwoPi;
      const glm::vec3 normal(r * std::cos(theta), r * std::sin(theta), z);
      mesh.vertices.push_back(
          {.position = normal * radius, .normal = normal, .color = color});
    }
  }
  const uint32_t stride = segments + 1;
  mesh.indices.reserve(static_cast<size_t>(segments) * rings * 6);
  for (uint32_t ring = 0; ring < rings; ++ring) {
    for (uint32_t seg = 0; seg < segments; ++seg) {
      const uint32_t a = ring * stride + seg;
      const uint32_t b = a + stride;
      mesh.indices.insert(mesh.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
    }
  }
  return mesh;
}

MeshData cylinder(float radius, float height, uint32_t segments,
                  const glm::vec4& color) {
  MeshData mesh;
  segments = segments < 3 ? 3 : segments;
  const float h = height * 0.5F;
  // Side: two rings of vertices with outward normals.
  for (uint32_t seg = 0; seg <= segments; ++seg) {
    const float theta =
        static_cast<float>(seg) / static_cast<float>(segments) * kTwoPi;
    const glm::vec3 normal(std::cos(theta), std::sin(theta), 0.0F);
    const glm::vec3 rim = normal * radius;
    mesh.vertices.push_back(
        {.position = {rim.x, rim.y, -h}, .normal = normal, .color = color});
    mesh.vertices.push_back(
        {.position = {rim.x, rim.y, h}, .normal = normal, .color = color});
  }
  for (uint32_t seg = 0; seg < segments; ++seg) {
    const uint32_t a = seg * 2;
    mesh.indices.insert(mesh.indices.end(),
                        {a, a + 2, a + 1, a + 1, a + 2, a + 3});
  }
  // Caps: a centre vertex plus the rim, per cap.
  for (const float z : {h, -h}) {
    const glm::vec3 normal(0.0F, 0.0F, z > 0.0F ? 1.0F : -1.0F);
    const auto centre = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(
        {.position = {0.0F, 0.0F, z}, .normal = normal, .color = color});
    for (uint32_t seg = 0; seg <= segments; ++seg) {
      const float theta =
          static_cast<float>(seg) / static_cast<float>(segments) * kTwoPi;
      mesh.vertices.push_back(
          {.position = {radius * std::cos(theta), radius * std::sin(theta), z},
           .normal = normal,
           .color = color});
    }
    for (uint32_t seg = 0; seg < segments; ++seg) {
      const uint32_t a = centre + 1 + seg;
      if (z > 0.0F) {
        mesh.indices.insert(mesh.indices.end(), {centre, a, a + 1});
      } else {
        mesh.indices.insert(mesh.indices.end(), {centre, a + 1, a});
      }
    }
  }
  return mesh;
}

void append(MeshData& mesh, const MeshData& other) {
  const auto base = static_cast<uint32_t>(mesh.vertices.size());
  mesh.vertices.insert(mesh.vertices.end(), other.vertices.begin(),
                       other.vertices.end());
  mesh.indices.reserve(mesh.indices.size() + other.indices.size());
  for (const uint32_t index : other.indices) {
    mesh.indices.push_back(base + index);
  }
}

void applyTransform(MeshData& mesh, const glm::mat4& transform) {
  const glm::mat3 normalMatrix =
      glm::transpose(glm::inverse(glm::mat3(transform)));
  for (Vertex& vertex : mesh.vertices) {
    vertex.position = glm::vec3(transform * glm::vec4(vertex.position, 1.0F));
    vertex.normal = glm::normalize(normalMatrix * vertex.normal);
  }
}

}  // namespace avalon::shapes

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

#include <gtest/gtest.h>

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/geometry/Shapes.h"
#include "Avalon/src/geometry/Vertex.h"

namespace avalon {

namespace {
constexpr float kEps = 1e-4F;

void expectValidIndexedMesh(const MeshData& mesh) {
  ASSERT_FALSE(mesh.empty());
  EXPECT_EQ(mesh.indices.size() % 3, 0U);
  for (const uint32_t index : mesh.indices) {
    EXPECT_LT(index, mesh.vertices.size());
  }
  for (const Vertex& vertex : mesh.vertices) {
    EXPECT_NEAR(glm::length(vertex.normal), 1.0F, kEps);
  }
}
}  // namespace

TEST(VertexTest, DescribesThreeAttributesInOneBinding) {
  const VkVertexInputBindingDescription binding = Vertex::bindingDescription();
  EXPECT_EQ(binding.binding, 0U);
  EXPECT_EQ(binding.stride, sizeof(Vertex));
  const auto attributes = Vertex::attributeDescriptions();
  ASSERT_EQ(attributes.size(), 3U);
  EXPECT_EQ(attributes[0].offset, 0U);
  EXPECT_EQ(attributes[1].offset, sizeof(glm::vec3));
  EXPECT_EQ(attributes[2].offset, 2 * sizeof(glm::vec3));
  EXPECT_EQ(attributes[2].format, VK_FORMAT_R32G32B32A32_SFLOAT);
}

TEST(ShapesTest, BoxHasSixFacesWithOutwardNormals) {
  const MeshData box = shapes::box(glm::vec3(2.0F, 4.0F, 6.0F));
  expectValidIndexedMesh(box);
  EXPECT_EQ(box.vertices.size(), 24U);
  EXPECT_EQ(box.indices.size(), 36U);
  EXPECT_EQ(box.triangleCount(), 12U);
  for (const Vertex& vertex : box.vertices) {
    EXPECT_LE(std::abs(vertex.position.x), 1.0F + kEps);
    EXPECT_LE(std::abs(vertex.position.y), 2.0F + kEps);
    EXPECT_LE(std::abs(vertex.position.z), 3.0F + kEps);
    // Each face normal points the same way as that face's offset.
    EXPECT_GT(glm::dot(vertex.normal, vertex.position), 0.0F);
  }
}

TEST(ShapesTest, SphereVerticesLieOnTheRadius) {
  const MeshData sphere = shapes::sphere(2.0F, 8, 4);
  expectValidIndexedMesh(sphere);
  EXPECT_EQ(sphere.vertices.size(), 9U * 5U);
  EXPECT_EQ(sphere.indices.size(), 8U * 4U * 6U);
  for (const Vertex& vertex : sphere.vertices) {
    EXPECT_NEAR(glm::length(vertex.position), 2.0F, kEps);
    EXPECT_NEAR(glm::dot(vertex.normal, glm::normalize(vertex.position)), 1.0F,
                kEps);
  }
}

TEST(ShapesTest, SphereClampsDegenerateResolutions) {
  const MeshData sphere = shapes::sphere(1.0F, 1, 1);
  expectValidIndexedMesh(sphere);
  EXPECT_EQ(sphere.vertices.size(), 4U * 3U);  // 3 segments, 2 rings
}

TEST(ShapesTest, CylinderIsClosed) {
  const MeshData cylinder = shapes::cylinder(1.0F, 2.0F, 6);
  expectValidIndexedMesh(cylinder);
  // Side: 6 quads; caps: 6 triangles each.
  EXPECT_EQ(cylinder.triangleCount(), 6U * 2U + 6U * 2U);
  for (const Vertex& vertex : cylinder.vertices) {
    EXPECT_LE(std::abs(vertex.position.z), 1.0F + kEps);
  }
}

TEST(ShapesTest, AppendOffsetsIndices) {
  MeshData mesh = shapes::box();
  const MeshData other = shapes::box();
  shapes::append(mesh, other);
  EXPECT_EQ(mesh.vertices.size(), 48U);
  EXPECT_EQ(mesh.indices.size(), 72U);
  for (size_t i = 36; i < mesh.indices.size(); ++i) {
    EXPECT_GE(mesh.indices[i], 24U);
    EXPECT_LT(mesh.indices[i], 48U);
  }
}

TEST(ShapesTest, TransformMovesPositionsAndRotatesNormals) {
  MeshData mesh = shapes::box(glm::vec3(1.0F));
  const glm::mat4 transform =
      glm::translate(glm::mat4(1.0F), glm::vec3(10.0F, 0.0F, 0.0F)) *
      glm::rotate(glm::mat4(1.0F), glm::half_pi<float>(),
                  glm::vec3(0.0F, 0.0F, 1.0F));
  shapes::applyTransform(mesh, transform);
  expectValidIndexedMesh(mesh);
  for (const Vertex& vertex : mesh.vertices) {
    EXPECT_NEAR(std::abs(vertex.position.x - 10.0F), 0.5F, kEps);
  }
}

}  // namespace avalon

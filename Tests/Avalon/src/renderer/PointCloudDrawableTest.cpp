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

#include <array>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "Avalon/src/geometry/Vertex.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/PointCloudDrawable.h"
#include "Avalon/src/renderer/Renderer.h"

namespace avalon {

class PointCloudDrawableTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }

  [[nodiscard]] std::shared_ptr<PointCloudDrawable> make(
      std::span<const PointVertex> points = {}) const {
    return std::make_shared<PointCloudDrawable>(avalon.getDevice(),
                                                avalon.getAllocator(), points);
  }
};

TEST(PointVertexTest, DescribesTwoAttributes) {
  EXPECT_EQ(PointVertex::bindingDescription().stride, sizeof(PointVertex));
  const auto attributes = PointVertex::attributeDescriptions();
  ASSERT_EQ(attributes.size(), 2U);
  EXPECT_EQ(attributes[0].format, VK_FORMAT_R32G32B32_SFLOAT);
  EXPECT_EQ(attributes[1].format, VK_FORMAT_R32G32B32A32_SFLOAT);
  EXPECT_EQ(attributes[1].offset, sizeof(glm::vec3));
}

TEST_F(PointCloudDrawableTest, StartsEmptyWithMinimumCapacity) {
  const std::shared_ptr<PointCloudDrawable> cloud = make();
  EXPECT_EQ(cloud->pointCount(), 0U);
  EXPECT_EQ(cloud->capacity(), PointCloudDrawable::kMinCapacity);
  EXPECT_TRUE(cloud->isVisible());
  EXPECT_EQ(cloud->getTint(), glm::vec4(1.0F));
  EXPECT_THROW(PointCloudDrawable(nullptr, avalon.getAllocator()),
               std::runtime_error);
}

TEST_F(PointCloudDrawableTest, SinglePointIsDrawn) {
  Renderer* renderer = avalon.getRenderer();
  const std::shared_ptr<PointCloudDrawable> cloud = make();
  cloud->setPoint({1.0F, 2.0F, 3.0F}, {1.0F, 0.0F, 0.0F, 1.0F});
  cloud->setPointSize(8.0F);
  EXPECT_EQ(cloud->pointCount(), 1U);
  EXPECT_EQ(cloud->getPointSize(), 8.0F);
  renderer->addDrawable(cloud);
  EXPECT_FALSE(
      renderer->getPipelineManager().has(PointCloudDrawable::kPipeline));
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(
      renderer->getPipelineManager().has(PointCloudDrawable::kPipeline));
  EXPECT_TRUE(avalon.frame());
}

TEST_F(PointCloudDrawableTest, SetPointsGrowsTheBufferGeometrically) {
  const std::shared_ptr<PointCloudDrawable> cloud = make();
  std::vector<PointVertex> points(PointCloudDrawable::kMinCapacity + 1);
  cloud->setPoints(points);
  EXPECT_EQ(cloud->pointCount(), points.size());
  EXPECT_EQ(cloud->capacity(), 2 * PointCloudDrawable::kMinCapacity);
  points.resize(static_cast<size_t>(5) * PointCloudDrawable::kMinCapacity);
  cloud->setPoints(points);
  EXPECT_EQ(cloud->capacity(), 8 * PointCloudDrawable::kMinCapacity);
  // Shrinking keeps the buffer.
  cloud->setPoints({});
  EXPECT_EQ(cloud->pointCount(), 0U);
  EXPECT_EQ(cloud->capacity(), 8 * PointCloudDrawable::kMinCapacity);
  // Drawing an empty cloud is a no-op, not an error.
  avalon.getRenderer()->addDrawable(cloud);
  EXPECT_TRUE(avalon.frame());
}

TEST_F(PointCloudDrawableTest, ConstructsFromPointsAndUpdatesWhileDrawing) {
  const std::array<PointVertex, 3> initial = {
      PointVertex{.position = {0.0F, 0.0F, 0.0F}, .color = glm::vec4(1.0F)},
      PointVertex{.position = {1.0F, 0.0F, 0.0F}, .color = glm::vec4(1.0F)},
      PointVertex{.position = {0.0F, 1.0F, 0.0F}, .color = glm::vec4(1.0F)}};
  const std::shared_ptr<PointCloudDrawable> cloud = make(initial);
  EXPECT_EQ(cloud->pointCount(), 3U);
  Renderer* renderer = avalon.getRenderer();
  renderer->addDrawable(cloud);
  EXPECT_TRUE(avalon.frame());
  cloud->setPoint({5.0F, 5.0F, 5.0F}, {0.0F, 1.0F, 0.0F, 1.0F});
  cloud->setTint({0.5F, 0.5F, 0.5F, 1.0F});
  cloud->setVisible(false);
  EXPECT_TRUE(avalon.frame());
  cloud->setVisible(true);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->removeDrawable(cloud));
}

}  // namespace avalon

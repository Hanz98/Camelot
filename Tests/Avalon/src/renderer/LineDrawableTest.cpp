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
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/geometry/Vertex.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/LineDrawable.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/VideoTexture.h"

namespace avalon {

namespace {

PointVertex vertex(float x, float y, float z) {
  return PointVertex{.position = {x, y, z}, .color = glm::vec4(1.0F)};
}

// A unit square in the xy plane, as a strip of four corners.
std::vector<PointVertex> square() {
  return {vertex(0.0F, 0.0F, 0.0F), vertex(1.0F, 0.0F, 0.0F),
          vertex(1.0F, 1.0F, 0.0F), vertex(0.0F, 1.0F, 0.0F)};
}

}  // namespace

// Draws line lists through the full frame loop on the mock ICD.
class LineDrawableTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }

  [[nodiscard]] std::shared_ptr<LineDrawable> make(
      std::span<const PointVertex> lines = {}) const {
    return std::make_shared<LineDrawable>(avalon.getDevice(),
                                          avalon.getAllocator(), lines);
  }

  // What the device allows: the mock ICD enables wideLines with a range of
  // [1, 8] and a granularity of 1; a real driver may differ.
  [[nodiscard]] bool wideLines() const {
    return avalon.getDevice()->getVkbPhysicalDevice().features.wideLines ==
           VK_TRUE;
  }
  [[nodiscard]] const VkPhysicalDeviceLimits& limits() const {
    return avalon.getDevice()->getVkbPhysicalDevice().properties.limits;
  }
};

TEST_F(LineDrawableTest, StartsEmptyWithDefaults) {
  const std::shared_ptr<LineDrawable> lines = make();
  EXPECT_EQ(lines->vertexCount(), 0U);
  EXPECT_EQ(lines->segmentCount(), 0U);
  EXPECT_EQ(lines->capacity(), LineDrawable::kMinCapacity);
  EXPECT_TRUE(lines->isVisible());
  EXPECT_EQ(lines->getTint(), glm::vec4(1.0F));
  EXPECT_EQ(lines->getTransform(), glm::mat4(1.0F));
  EXPECT_EQ(lines->getLineWidth(), 1.0F);
  EXPECT_EQ(lines->pipelineName(), LineDrawable::kPipeline);
  EXPECT_THROW(LineDrawable(nullptr, avalon.getAllocator()),
               std::runtime_error);
  EXPECT_THROW(LineDrawable(avalon.getDevice(), nullptr), std::runtime_error);
}

TEST_F(LineDrawableTest, SetLinesGrowsTheBufferAndRejectsOddCounts) {
  const std::shared_ptr<LineDrawable> lines = make();
  std::vector<PointVertex> pairs(LineDrawable::kMinCapacity + 2);
  lines->setLines(pairs);
  EXPECT_EQ(lines->vertexCount(), pairs.size());
  EXPECT_EQ(lines->segmentCount(), pairs.size() / 2);
  EXPECT_EQ(lines->capacity(), 2 * LineDrawable::kMinCapacity);
  pairs.resize(static_cast<size_t>(5) * LineDrawable::kMinCapacity);
  lines->setLines(pairs);
  EXPECT_EQ(lines->capacity(), 8 * LineDrawable::kMinCapacity);

  // An odd count is rejected and leaves the drawable untouched.
  const std::array<PointVertex, 3> odd = {vertex(0.0F, 0.0F, 0.0F),
                                          vertex(1.0F, 0.0F, 0.0F),
                                          vertex(2.0F, 0.0F, 0.0F)};
  EXPECT_THROW(lines->setLines(odd), std::runtime_error);
  EXPECT_EQ(lines->vertexCount(), pairs.size());
  EXPECT_THROW((void)make(odd), std::runtime_error);

  // Shrinking keeps the buffer; an empty set draws nothing.
  lines->setLines({});
  EXPECT_EQ(lines->vertexCount(), 0U);
  EXPECT_EQ(lines->capacity(), 8 * LineDrawable::kMinCapacity);
  avalon.getRenderer()->addDrawable(lines);
  EXPECT_TRUE(avalon.frame());
}

TEST_F(LineDrawableTest, SetStripExpandsToPairs) {
  const std::shared_ptr<LineDrawable> lines = make();
  const std::vector<PointVertex> corners = square();
  lines->setStrip(corners, false);
  EXPECT_EQ(lines->vertexCount(), 6U);  // three segments
  EXPECT_EQ(lines->segmentCount(), 3U);
  lines->setStrip(corners, true);
  EXPECT_EQ(lines->vertexCount(), 8U);  // plus the closing segment
  EXPECT_EQ(lines->segmentCount(), 4U);

  // Two points are one segment whether or not the strip is closed.
  const std::array<PointVertex, 2> two = {vertex(0.0F, 0.0F, 0.0F),
                                          vertex(1.0F, 0.0F, 0.0F)};
  lines->setStrip(two, true);
  EXPECT_EQ(lines->vertexCount(), 2U);
  // Fewer than two points draw nothing.
  const std::array<PointVertex, 1> one = {vertex(0.0F, 0.0F, 0.0F)};
  lines->setStrip(one, false);
  EXPECT_EQ(lines->vertexCount(), 0U);
  lines->setStrip({}, true);
  EXPECT_EQ(lines->vertexCount(), 0U);
}

TEST_F(LineDrawableTest, TransformTintAndWidthRoundTrip) {
  const std::shared_ptr<LineDrawable> lines = make();
  const glm::mat4 model =
      glm::translate(glm::mat4(1.0F), glm::vec3(1.0F, 2.0F, 3.0F));
  lines->setTransform(model);
  EXPECT_EQ(lines->getTransform(), model);
  lines->setTint({1.0F, 0.5F, 0.0F, 0.25F});
  EXPECT_EQ(lines->getTint(), glm::vec4(1.0F, 0.5F, 0.0F, 0.25F));
  lines->setVisible(false);
  EXPECT_FALSE(lines->isVisible());

  // Widths follow the device limits: clamped to lineWidthRange when
  // wideLines is on, always 1.0 otherwise.
  lines->setLineWidth(1.0F);
  EXPECT_EQ(lines->getLineWidth(), 1.0F);
  EXPECT_EQ(lines->pipelineName(), LineDrawable::kPipeline);
  lines->setLineWidth(1000.0F);
  if (wideLines()) {
    EXPECT_EQ(lines->getLineWidth(), limits().lineWidthRange[1]);
    EXPECT_NE(lines->pipelineName(), LineDrawable::kPipeline);
    lines->setLineWidth(2.0F);
    EXPECT_EQ(lines->getLineWidth(), 2.0F);
    EXPECT_EQ(lines->pipelineName(), "lines@2");
    EXPECT_EQ(LineDrawable::pipelineNameFor(2.5F), "lines@2.5");
  } else {
    EXPECT_EQ(lines->getLineWidth(), 1.0F);
    EXPECT_EQ(lines->pipelineName(), LineDrawable::kPipeline);
  }
  lines->setLineWidth(-4.0F);
  EXPECT_GE(lines->getLineWidth(), limits().lineWidthRange[0]);
  // NaN falls back to the default.
  lines->setLineWidth(std::numeric_limits<float>::quiet_NaN());
  EXPECT_EQ(lines->getLineWidth(), 1.0F);
}

TEST_F(LineDrawableTest, RendererDrawsLinesAndBuildsThePipeline) {
  Renderer* renderer = avalon.getRenderer();
  const std::vector<PointVertex> corners = square();
  const std::shared_ptr<LineDrawable> outline = make();
  outline->setStrip(corners, true);
  const std::shared_ptr<LineDrawable> wide = make();
  wide->setStrip(corners, false);
  wide->setTint({1.0F, 1.0F, 1.0F, 0.5F});
  wide->setLineWidth(3.0F);
  renderer->addDrawable(outline);
  renderer->addDrawable(wide);
  EXPECT_FALSE(renderer->getPipelineManager().has(LineDrawable::kPipeline));

  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(avalon.frame());
  }
  EXPECT_TRUE(renderer->getPipelineManager().has(LineDrawable::kPipeline));
  EXPECT_TRUE(renderer->getPipelineManager().has(wide->pipelineName()));
  EXPECT_TRUE(renderer->getPipelineManager().has(outline->pipelineName()));

  // Updating while drawing, hiding, and swapchain recreation keep working.
  outline->setLines(corners);  // two segments now
  EXPECT_EQ(outline->segmentCount(), 2U);
  wide->setVisible(false);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->recreateSwapchain());
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->removeDrawable(outline));
  EXPECT_TRUE(renderer->removeDrawable(wide));
  EXPECT_TRUE(avalon.frame());
}

TEST_F(LineDrawableTest, DrawsAsVideoOverlayInPixelSpace) {
  const auto texture = std::make_shared<VideoTexture>(
      avalon.getDevice(), avalon.getAllocator(), avalon.getUi(), 32, 16,
      Renderer::kFramesInFlight);
  Renderer* renderer = avalon.getRenderer();
  renderer->addPrePass(texture);
  std::vector<std::byte> rgba(static_cast<size_t>(32) * 16 * 4, std::byte{80});
  texture->upload(rgba);

  // A 2D box annotation in pixel coordinates.
  const std::shared_ptr<LineDrawable> box = make();
  const std::array<PointVertex, 4> corners = {
      vertex(4.0F, 2.0F, 0.0F), vertex(20.0F, 2.0F, 0.0F),
      vertex(20.0F, 12.0F, 0.0F), vertex(4.0F, 12.0F, 0.0F)};
  box->setStrip(corners, true);
  box->setLineWidth(2.0F);
  texture->addOverlay(box);
  EXPECT_EQ(texture->overlayCount(), 1U);

  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(texture->currentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  EXPECT_TRUE(avalon.frame());
  // The overlay pass has its own pipeline cache; the main one stays clean.
  EXPECT_FALSE(renderer->getPipelineManager().has(box->pipelineName()));
  EXPECT_TRUE(texture->removeOverlay(box));
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->removePrePass(texture));
}

}  // namespace avalon

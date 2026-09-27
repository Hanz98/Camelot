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
#include <imgui.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "Avalon/src/geometry/Shapes.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/PointCloudDrawable.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/TestPatternSource.h"
#include "Avalon/src/ui/VideoTexture.h"
#include "Avalon/src/ui/VideoWidget.h"

namespace avalon {

TEST(TestPatternSourceTest, ProducesDeterministicFrames) {
  TestPatternSource source(64, 32, 10.0, 3);
  EXPECT_EQ(source.width(), 64U);
  EXPECT_EQ(source.height(), 32U);
  EXPECT_EQ(source.frameBytes(), 64U * 32U * 4U);
  std::vector<std::byte> a(source.frameBytes());
  std::vector<std::byte> b(source.frameBytes());
  double t = -1.0;
  EXPECT_TRUE(source.nextFrame(a, t));
  EXPECT_DOUBLE_EQ(t, 0.0);
  EXPECT_TRUE(source.nextFrame(b, t));
  EXPECT_DOUBLE_EQ(t, 0.1);
  EXPECT_NE(a, b);  // the bar moved
  std::vector<std::byte> again(source.frameBytes());
  source.render(0, again);
  EXPECT_EQ(again, a);
  EXPECT_TRUE(source.nextFrame(a, t));
  EXPECT_FALSE(source.nextFrame(a, t));  // frame limit reached
  source.rewind();
  EXPECT_EQ(source.frameIndex(), 0U);
  EXPECT_TRUE(source.nextFrame(a, t));
  EXPECT_EQ(source.barX(0), 0U);
  EXPECT_EQ(source.barX(1), 4U);
  EXPECT_EQ(source.barX(16), 0U);  // wraps at the width
  // Alpha is always opaque.
  EXPECT_EQ(a[3], std::byte{255});
  EXPECT_THROW(TestPatternSource(0, 1), std::runtime_error);
  std::vector<std::byte> small(4);
  EXPECT_THROW(source.render(0, small), std::runtime_error);
}

class VideoTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }

  [[nodiscard]] std::shared_ptr<VideoTexture> makeTexture(uint32_t w,
                                                          uint32_t h) const {
    return std::make_shared<VideoTexture>(avalon.getDevice(),
                                          avalon.getAllocator(), avalon.getUi(),
                                          w, h, Renderer::kFramesInFlight);
  }
};

TEST_F(VideoTest, TextureUploadsAndRunsTheOverlayPass) {
  const std::shared_ptr<VideoTexture> texture = makeTexture(32, 16);
  EXPECT_NE(texture->uiTexture(), VK_NULL_HANDLE);
  EXPECT_NE(texture->overlayRenderPass(), VK_NULL_HANDLE);
  EXPECT_FALSE(texture->hasFrame());
  EXPECT_EQ(texture->currentLayout(), VK_IMAGE_LAYOUT_UNDEFINED);

  Renderer* renderer = avalon.getRenderer();
  renderer->addPrePass(texture);
  EXPECT_EQ(renderer->prePassCount(), 1U);
  EXPECT_TRUE(avalon.frame());  // no frame yet: pre-pass is a no-op
  EXPECT_EQ(texture->currentLayout(), VK_IMAGE_LAYOUT_UNDEFINED);

  std::vector<std::byte> rgba(static_cast<size_t>(32) * 16 * 4, std::byte{200});
  texture->upload(rgba);
  EXPECT_TRUE(texture->hasFrame());
  EXPECT_EQ(texture->uploadCount(), 1U);
  EXPECT_THROW(texture->upload(std::vector<std::byte>(7)), std::runtime_error);

  auto box = std::make_shared<MeshDrawable>(
      avalon.getDevice(), avalon.getAllocator(), shapes::box(glm::vec3(8.0F)));
  auto marker = std::make_shared<PointCloudDrawable>(avalon.getDevice(),
                                                     avalon.getAllocator());
  marker->setPoint({16.0F, 8.0F, 0.0F}, glm::vec4(1.0F));
  texture->addOverlay(box);
  texture->addOverlay(marker);
  EXPECT_EQ(texture->overlayCount(), 2U);
  EXPECT_THROW(texture->addOverlay(nullptr), std::runtime_error);

  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(texture->currentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  EXPECT_TRUE(avalon.frame());  // re-upload + overlay every frame

  EXPECT_TRUE(texture->removeOverlay(box));
  EXPECT_FALSE(texture->removeOverlay(box));
  texture->clearOverlays();
  EXPECT_EQ(texture->overlayCount(), 0U);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->removePrePass(texture));
  EXPECT_FALSE(renderer->removePrePass(texture));
  EXPECT_THROW(renderer->addPrePass(nullptr), std::runtime_error);
}

TEST_F(VideoTest, TextureRejectsBadArguments) {
  EXPECT_THROW((void)makeTexture(0, 4), std::runtime_error);
  EXPECT_THROW(
      VideoTexture(nullptr, avalon.getAllocator(), avalon.getUi(), 4, 4, 2),
      std::runtime_error);
  EXPECT_THROW(
      VideoTexture(avalon.getDevice(), avalon.getAllocator(), nullptr, 4, 4, 2),
      std::runtime_error);
}

TEST_F(VideoTest, WidgetPullsFramesAtTheSourceRate) {
  const std::shared_ptr<VideoTexture> texture = makeTexture(16, 8);
  auto source = std::make_shared<TestPatternSource>(16, 8, 10.0, 5);
  VideoWidget widget("clip", source, texture);
  EXPECT_TRUE(widget.isPlaying());
  widget.tick(0.0);  // the first frame is due immediately
  EXPECT_EQ(widget.framesShown(), 1U);
  widget.tick(0.05);  // not yet
  EXPECT_EQ(widget.framesShown(), 1U);
  widget.tick(0.05);  // t = 0.1: second frame
  EXPECT_EQ(widget.framesShown(), 2U);
  EXPECT_DOUBLE_EQ(widget.timestamp(), 0.1);
  widget.pause();
  widget.tick(1.0);
  EXPECT_EQ(widget.framesShown(), 2U);
  widget.play();
  widget.tick(1.0);  // a stall pulls at most four frames
  EXPECT_EQ(widget.framesShown(), 5U);
  EXPECT_TRUE(widget.hasEnded());
  widget.rewind();
  EXPECT_FALSE(widget.hasEnded());
  widget.tick(0.0);
  EXPECT_EQ(widget.framesShown(), 6U);
  EXPECT_EQ(texture->uploadCount(), 6U);

  // Mismatched sizes are rejected.
  auto other = std::make_shared<TestPatternSource>(8, 8);
  EXPECT_THROW(VideoWidget("bad", other, texture), std::runtime_error);
  EXPECT_THROW(VideoWidget("bad", nullptr, texture), std::runtime_error);
}

TEST_F(VideoTest, WidgetDrawsThroughTheFrameLoop) {
  const std::shared_ptr<VideoTexture> texture = makeTexture(16, 8);
  auto source = std::make_shared<TestPatternSource>(16, 8, 30.0);
  VideoWidget widget("clip", source, texture);
  avalon.getRenderer()->addPrePass(texture);
  avalon.setUiCallback([&]() {
    widget.tick(1.0 / 60.0);
    ImGui::Begin("video");
    widget.draw();
    ImGui::End();
  });
  EXPECT_TRUE(avalon.frame());  // draw() before the first upload reached GPU
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(avalon.frame());
  EXPECT_GE(widget.framesShown(), 1U);
  EXPECT_EQ(texture->currentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  avalon.getRenderer()->removePrePass(texture);
}

}  // namespace avalon

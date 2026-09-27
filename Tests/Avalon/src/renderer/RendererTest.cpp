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
#include <cstdlib>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/Renderer.h"

namespace avalon {

// Drives the full frame loop through Avalon. Runs on the mock ICD in CI.
class RendererTest : public testing::Test {
 protected:
  Avalon avalon;

  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }
};

TEST_F(RendererTest, InitCreatesRenderer) {
  ASSERT_TRUE(avalon.isInitialized());
  ASSERT_NE(avalon.getRenderer(), nullptr);
  EXPECT_EQ(avalon.getRenderer()->getFrameCount(), 0U);
  EXPECT_NE(avalon.getRenderer()->getRenderPass().getRenderPass(),
            VK_NULL_HANDLE);
}

TEST_F(RendererTest, DrawsThreeFrames) {
  Renderer* renderer = avalon.getRenderer();
  ASSERT_NE(renderer, nullptr);
  renderer->setClearColor(0.2F, 0.3F, 0.4F);
  constexpr uint64_t kFrames = 3;
  for (uint64_t i = 0; i < kFrames; ++i) {
    // frame() polls events and draws; the window is not being closed.
    EXPECT_TRUE(avalon.frame());
  }
  EXPECT_EQ(renderer->getFrameCount(), kFrames);
}

TEST_F(RendererTest, SwapchainRecreationKeepsDrawing) {
  Renderer* renderer = avalon.getRenderer();
  ASSERT_NE(renderer, nullptr);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->recreateSwapchain());
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(renderer->getFrameCount(), 2U);
}

TEST_F(RendererTest, ShouldCloseIsFalseWhileOpen) {
  EXPECT_FALSE(avalon.shouldClose());
}

TEST_F(RendererTest, WindowResizeRecreatesSwapchainAndKeepsDrawing) {
  Renderer* renderer = avalon.getRenderer();
  ASSERT_NE(renderer, nullptr);
  EXPECT_TRUE(avalon.frame());

  constexpr int kWidth = 640;
  constexpr int kHeight = 480;
  glfwSetWindowSize(avalon.getWindow()->getWindow(), kWidth, kHeight);
  // Two frames: the first sees the resize flag and rebuilds, the second draws
  // into the rebuilt swapchain.
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(avalon.frame());
  EXPECT_GE(renderer->getFrameCount(), 2U);

  // The mock ICD reports a fixed surface extent, so only check the size on a
  // real driver.
  if (std::getenv("CAMELOT_TESTS_USE_MOCK_ICD") == nullptr) {
    const auto [fbWidth, fbHeight] = avalon.getWindow()->getFramebufferSize();
    const VkExtent2D extent = renderer->getSwapchainExtent();
    EXPECT_EQ(extent.width, fbWidth);
    EXPECT_EQ(extent.height, fbHeight);
  }
}

}  // namespace avalon

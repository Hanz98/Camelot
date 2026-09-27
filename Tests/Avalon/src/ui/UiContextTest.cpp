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

#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include <imgui.h>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/ui/UiContext.h"

namespace avalon {

// The ImGui backends run on the mock ICD through the full frame loop.
class UiContextTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }
};

TEST_F(UiContextTest, EngineCreatesAndRendersTheUi) {
  UiContext* ui = avalon.getUi();
  ASSERT_NE(ui, nullptr);
  EXPECT_EQ(avalon.getRenderer()->getUi(), ui);
  EXPECT_FALSE(ui->isFrameOpen());
  int callbacks = 0;
  avalon.setUiCallback([&]() {
    ++callbacks;
    EXPECT_TRUE(ui->isFrameOpen());
    ImGui::Begin("test");
    ImGui::TextUnformatted("hello");
    ImGui::End();
  });
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(callbacks, 2);
  EXPECT_EQ(ui->renderedFrames(), 2U);
  EXPECT_FALSE(ui->isFrameOpen());
  EXPECT_FALSE(ui->wantsMouse());
  EXPECT_FALSE(ui->wantsKeyboard());
}

TEST_F(UiContextTest, DiscardedFramesDoNotBreakTheNext) {
  UiContext* ui = avalon.getUi();
  ui->newFrame();
  EXPECT_TRUE(ui->isFrameOpen());
  ui->discardFrame();
  EXPECT_FALSE(ui->isFrameOpen());
  ui->newFrame();
  ui->newFrame();  // an open frame is discarded first
  EXPECT_TRUE(ui->isFrameOpen());
  ui->discardFrame();
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(ui->renderedFrames(), 1U);
}

TEST_F(UiContextTest, SwapchainRecreationKeepsRenderingTheUi) {
  avalon.setUiCallback([]() {
    ImGui::Begin("after resize");
    ImGui::End();
  });
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(avalon.getRenderer()->recreateSwapchain());
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(avalon.getUi()->renderedFrames(), 2U);
}

TEST_F(UiContextTest, CameraIgnoresInputWhileTheUiWantsTheMouse) {
  CameraController* controller = avalon.getCameraController();
  ASSERT_NE(controller, nullptr);
  EXPECT_FALSE(controller->isInputBlocked());
  bool blocked = true;
  controller->setInputBlocked([&]() { return blocked; });
  const float distance = avalon.getCamera()->getDistance();
  controller->onScroll(1.0);
  EXPECT_EQ(avalon.getCamera()->getDistance(), distance);
  controller->onMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0.0, 0.0);
  EXPECT_FALSE(controller->isOrbiting());
  blocked = false;
  controller->onScroll(1.0);
  EXPECT_LT(avalon.getCamera()->getDistance(), distance);
}

}  // namespace avalon

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

#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>

#include "Avalon/src/geometry/Shapes.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/RenderTarget.h"
#include "Avalon/src/ui/SceneViewWidget.h"

namespace avalon {

class SceneViewTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }

  [[nodiscard]] std::shared_ptr<SceneViewWidget> makeView(
      uint32_t w = 64, uint32_t h = 48) const {
    return std::make_shared<SceneViewWidget>(
        "view", avalon.getDevice(), avalon.getAllocator(), avalon.getUi(),
        avalon.getRenderer(), Renderer::kFramesInFlight, w, h);
  }
};

TEST_F(SceneViewTest, RenderTargetResizes) {
  RenderTarget target(avalon.getDevice(), avalon.getAllocator(), avalon.getUi(),
                      32, 16);
  EXPECT_EQ(target.width(), 32U);
  EXPECT_EQ(target.height(), 16U);
  EXPECT_NE(target.renderPass(), VK_NULL_HANDLE);
  EXPECT_NE(target.uiTexture(), VK_NULL_HANDLE);
  const uint64_t generation = target.generation();
  EXPECT_FALSE(target.resize(32, 16));  // unchanged
  EXPECT_FALSE(target.resize(0, 16));   // ignored
  EXPECT_EQ(target.generation(), generation);
  EXPECT_TRUE(target.resize(48, 24));
  EXPECT_EQ(target.extent().width, 48U);
  EXPECT_EQ(target.generation(), generation + 1);
  EXPECT_NE(target.uiTexture(), VK_NULL_HANDLE);
  EXPECT_THROW(RenderTarget(avalon.getDevice(), avalon.getAllocator(),
                            avalon.getUi(), 0, 4),
               std::runtime_error);
  EXPECT_THROW(
      RenderTarget(nullptr, avalon.getAllocator(), avalon.getUi(), 4, 4),
      std::runtime_error);
}

TEST_F(SceneViewTest, RendersTheSceneOffscreenEveryFrame) {
  Renderer* renderer = avalon.getRenderer();
  renderer->addDrawable(std::make_shared<MeshDrawable>(
      avalon.getDevice(), avalon.getAllocator(), shapes::box()));
  renderer->setDrawablesInMainPass(false);
  EXPECT_FALSE(renderer->drawablesInMainPass());

  const std::shared_ptr<SceneViewWidget> view = makeView();
  renderer->addPrePass(view);
  EXPECT_EQ(view->renderedFrames(), 0U);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(view->renderedFrames(), 2U);
  EXPECT_EQ(view->target().width(), 64U);

  // A size request is applied at the next pre-pass.
  view->requestSize(100, 80);
  EXPECT_EQ(view->target().width(), 64U);
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(view->target().width(), 100U);
  EXPECT_EQ(view->target().height(), 80U);
  view->requestSize(1, 1);  // clamped to the minimum
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(view->target().width(), SceneViewWidget::kMinSize);
  EXPECT_TRUE(renderer->removePrePass(view));
}

TEST_F(SceneViewTest, HasItsOwnCameraWithMouseControl) {
  const std::shared_ptr<SceneViewWidget> view = makeView();
  Camera& camera = view->camera();
  const float yaw = camera.getYaw();
  CameraController& controller = view->controller();
  controller.onMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 10.0, 10.0);
  controller.onCursorMove(30.0, 10.0);
  controller.onMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 30.0, 10.0);
  EXPECT_LT(camera.getYaw(), yaw);
  // The engine's window-level camera is untouched.
  EXPECT_NEAR(avalon.getCamera()->getYaw(), yaw, 1e-6F);
  const float distance = camera.getDistance();
  controller.onScroll(1.0);
  EXPECT_LT(camera.getDistance(), distance);
}

TEST_F(SceneViewTest, DrawsInsideAWindowAndFollowsItsSize) {
  const std::shared_ptr<SceneViewWidget> view = makeView();
  avalon.getRenderer()->addPrePass(view);
  avalon.setUiCallback([&]() {
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    ImGui::Begin("view window");
    view->draw();
    ImGui::End();
  });
  EXPECT_TRUE(avalon.frame());  // draw() requests the window's content size
  EXPECT_TRUE(avalon.frame());  // the pre-pass applies it
  EXPECT_GT(view->target().width(), 200U);
  EXPECT_LT(view->target().width(), 300U);
  EXPECT_GT(view->renderedFrames(), 0U);
  // isHovered() follows the real cursor, which on a CI desktop (Windows) can
  // sit inside the freshly created window, so it is deliberately not asserted.
  avalon.setUiCallback(nullptr);
  avalon.getRenderer()->removePrePass(view);
}

TEST_F(SceneViewTest, RejectsBadArguments) {
  EXPECT_THROW(SceneViewWidget("v", nullptr, avalon.getAllocator(),
                               avalon.getUi(), avalon.getRenderer(), 2),
               std::runtime_error);
  EXPECT_THROW(SceneViewWidget("v", avalon.getDevice(), avalon.getAllocator(),
                               avalon.getUi(), nullptr, 2),
               std::runtime_error);
}

}  // namespace avalon

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

#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/geometry/Shapes.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/Renderer.h"

namespace avalon {

// Draws meshes through the full frame loop on the mock ICD.
class MeshDrawableTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }

  [[nodiscard]] std::shared_ptr<MeshDrawable> make(const MeshData& mesh) const {
    return std::make_shared<MeshDrawable>(avalon.getDevice(),
                                          avalon.getAllocator(), mesh);
  }
};

TEST_F(MeshDrawableTest, UploadsTheMesh) {
  const std::shared_ptr<MeshDrawable> box = make(shapes::box());
  EXPECT_EQ(box->indexCount(), 36U);
  EXPECT_TRUE(box->isVisible());
  EXPECT_EQ(box->getTransform(), glm::mat4(1.0F));
  EXPECT_EQ(box->getColor(), glm::vec4(1.0F));
  EXPECT_THROW((void)make(MeshData{}), std::runtime_error);
}

TEST_F(MeshDrawableTest, RendererDrawsDrawablesAndBuildsTheMeshPipeline) {
  Renderer* renderer = avalon.getRenderer();
  const std::shared_ptr<MeshDrawable> box = make(shapes::box());
  const std::shared_ptr<MeshDrawable> sphere = make(shapes::sphere());
  sphere->setTransform(
      glm::translate(glm::mat4(1.0F), glm::vec3(2.0F, 0.0F, 0.0F)));
  sphere->setColor({0.0F, 1.0F, 0.0F, 1.0F});
  renderer->addDrawable(box);
  renderer->addDrawable(sphere);
  EXPECT_EQ(renderer->drawableCount(), 2U);
  EXPECT_FALSE(renderer->getPipelineManager().has(MeshDrawable::kPipeline));

  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(avalon.frame());
  }
  EXPECT_TRUE(renderer->getPipelineManager().has(MeshDrawable::kPipeline));
  EXPECT_EQ(renderer->getFrameCount(), 3U);

  // Camera changes and invisible drawables do not upset the loop.
  renderer->getCamera().orbit(0.3F, 0.1F);
  box->setVisible(false);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->recreateSwapchain());
  EXPECT_TRUE(avalon.frame());
}

TEST_F(MeshDrawableTest, RemoveAndClearDrawables) {
  Renderer* renderer = avalon.getRenderer();
  const std::shared_ptr<MeshDrawable> box = make(shapes::box());
  renderer->addDrawable(box);
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(renderer->removeDrawable(box));
  EXPECT_FALSE(renderer->removeDrawable(box));
  EXPECT_EQ(renderer->drawableCount(), 0U);
  renderer->addDrawable(make(shapes::cylinder()));
  renderer->addDrawable(make(shapes::sphere()));
  renderer->clearDrawables();
  EXPECT_EQ(renderer->drawableCount(), 0U);
  EXPECT_THROW(renderer->addDrawable(nullptr), std::runtime_error);
  EXPECT_TRUE(avalon.frame());
}

TEST_F(MeshDrawableTest, EngineWiresTheCameraController) {
  ASSERT_NE(avalon.getCamera(), nullptr);
  ASSERT_NE(avalon.getCameraController(), nullptr);
  EXPECT_EQ(avalon.getCameraController()->camera(), avalon.getCamera());
  EXPECT_TRUE(avalon.getWindow()->getInputHooks().onScroll);
}

}  // namespace avalon

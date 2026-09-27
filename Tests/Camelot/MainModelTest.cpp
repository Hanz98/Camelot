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

#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/TreeWidget.h"
#include "Camelot/src/main/MainModel.h"

namespace camelot {

// The application's demo scene and dockable UI run through the engine on the
// mock ICD.
TEST(MainModelTest, DemoSceneAndUiRunForAFewFrames) {
  MainModel model;
  model.engine().init();
  model.populateDemoScene();
  EXPECT_EQ(model.objects().size(), 5U);
  ASSERT_NE(model.points(), nullptr);
  EXPECT_EQ(avalon::TreeWidget::countNodes(model.sceneTree()), 8U);

  model.setupUi();
  EXPECT_FALSE(model.engine().getRenderer()->drawablesInMainPass());
  EXPECT_EQ(model.sceneViews().size(), 1U);
  EXPECT_EQ(model.graphs().size(), 1U);
  EXPECT_EQ(model.videos().size(), 1U);
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(model.engine().frame());
  }
  EXPECT_GE(model.graphs().front().graph->size(), 3U);
  EXPECT_GE(model.videos().front().widget->framesShown(), 1U);
  EXPECT_GE(model.sceneViews().front().view->renderedFrames(), 3U);

  // Toggling a tree node hides the object.
  avalon::TreeNode* objects = model.sceneTree().find("Objects");
  ASSERT_NE(objects, nullptr);
  avalon::TreeWidget::setVisible(*objects->find("red box"), false);
  EXPECT_FALSE(model.objects().front()->isVisible());
  EXPECT_TRUE(model.engine().frame());

  model.teardown();
  model.teardown();  // idempotent
  EXPECT_TRUE(model.engine().getRenderer()->drawablesInMainPass());
  model.engine().cleanUp();
}

TEST(MainModelTest, WidgetsCanBeOpenedSeveralTimesAndClosed) {
  MainModel model;
  model.engine().init();
  model.populateDemoScene();
  model.setupUi();
  avalon::Renderer* renderer = model.engine().getRenderer();
  const size_t prePasses = renderer->prePassCount();  // view + video

  MainModel::SceneViewWindow& second = model.addSceneView();
  model.addGraph();
  model.addVideo();
  EXPECT_EQ(model.sceneViews().size(), 2U);
  EXPECT_EQ(model.graphs().size(), 2U);
  EXPECT_EQ(model.videos().size(), 2U);
  EXPECT_NE(model.sceneViews()[0].title, model.sceneViews()[1].title);
  EXPECT_EQ(renderer->prePassCount(), prePasses + 2);
  // Each view has its own camera.
  second.view->camera().orbit(1.0F, 0.0F);
  EXPECT_NE(second.view->camera().getYaw(),
            model.sceneViews().front().view->camera().getYaw());
  for (int i = 0; i < 2; ++i) {
    EXPECT_TRUE(model.engine().frame());
  }
  EXPECT_GE(model.sceneViews()[1].view->renderedFrames(), 2U);

  model.sceneViews()[1].open = false;
  model.graphs()[0].open = false;
  model.videos()[1].open = false;
  model.pruneClosedWindows();
  EXPECT_EQ(model.sceneViews().size(), 1U);
  EXPECT_EQ(model.graphs().size(), 1U);
  EXPECT_EQ(model.videos().size(), 1U);
  EXPECT_EQ(renderer->prePassCount(), prePasses);
  EXPECT_TRUE(model.engine().frame());

  model.teardown();
  model.engine().cleanUp();
}

}  // namespace camelot

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

#include <cstddef>
#include <filesystem>  // NOLINT(build/c++17)
#include <memory>
#include <string>

#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/TreeWidget.h"
#include "Avalon/src/ui/VideoTexture.h"
#include "Camelot/src/data/FoxgloveMessages.h"
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

// Replays the fixture recording through the full UI on the mock ICD.
TEST(MainModelTest, OpensARecordingWithTopicsTimelineAndCameras) {
  const std::filesystem::path fixture =
      std::filesystem::path(CAMELOT_TEST_FIXTURES_DIR) / "nuscenes-mini.mcap";
  MainModel model;
  model.engine().init();
  model.populateDemoScene();

  // A bad path keeps the demo scene and reports the error for the modal.
  EXPECT_FALSE(model.openRecording(fixture.parent_path() / "missing.mcap"));
  EXPECT_EQ(model.recording(), nullptr);
  EXPECT_EQ(model.objects().size(), 5U);
  EXPECT_NE(model.openError().find("missing.mcap"), std::string::npos);

  ASSERT_TRUE(model.openRecording(fixture));
  ASSERT_NE(model.recording(), nullptr);
  ASSERT_NE(model.sceneUpdater(), nullptr);
  EXPECT_TRUE(model.objects().empty());  // the demo scene is gone
  EXPECT_EQ(model.points(), nullptr);
  EXPECT_TRUE(model.recording()->playback().isPlaying());
  EXPECT_EQ(model.sceneUpdater()->renderFrame(), "map");

  // The scene tree lists the supported topics, the disabled branch the rest.
  avalon::TreeNode* topics = model.sceneTree().find(MainModel::kTopicsBranch);
  ASSERT_NE(topics, nullptr);
  EXPECT_EQ(topics->children.size(), 11U);
  EXPECT_EQ(model.unsupportedTree().children.size(), 2U);
  EXPECT_NE(model.unsupportedTree().children.front().label.find(
                "foxglove.LocationFix"),
            std::string::npos);

  // One camera window per image topic, already showing the first frame
  // (the state at the start is loaded on open).
  EXPECT_EQ(model.cameraTopics().size(), 2U);
  model.setupUi();
  ASSERT_EQ(model.videos().size(), 2U);
  EXPECT_TRUE(model.videos().front().isCamera());
  EXPECT_EQ(model.videos().front().widget, nullptr);
  EXPECT_EQ(model.sceneUpdater()->cameraCount(), 2U);
  {
    // Scoped: a texture handle must not outlive the engine.
    const std::shared_ptr<avalon::VideoTexture> front =
        model.sceneUpdater()->cameraTexture("/CAM_FRONT/image_rect_compressed");
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->width(), 64U);
    EXPECT_TRUE(front->hasFrame());
  }
  EXPECT_EQ(model.sceneUpdater()->entityCount(), 4U);  // 3 markers + lanes
  EXPECT_EQ(model.sceneUpdater()->cloudCount(), 1U);   // no radar at T0

  for (int i = 0; i < 4; ++i) {
    EXPECT_TRUE(model.engine().frame());
  }
  EXPECT_GE(model.sceneViews().front().view->renderedFrames(), 4U);

  // Toggling a topic node hides its drawables.
  avalon::TreeNode* lidar = topics->find("/LIDAR_TOP");
  ASSERT_NE(lidar, nullptr);
  avalon::TreeWidget::setVisible(*lidar, false);
  EXPECT_FALSE(model.recording()->topic("/LIDAR_TOP")->visible);
  EXPECT_FALSE(
      model.sceneUpdater()->cloud("/LIDAR_TOP")->drawable->isVisible());
  EXPECT_TRUE(model.engine().frame());

  // A seek reloads the state at that time: radar has arrived by then.
  const Time start = model.recording()->playback().start();
  model.recording()->seek(start + 1'700'000'000ULL);
  EXPECT_EQ(model.sceneUpdater()->cloudCount(), 2U);
  EXPECT_EQ(model.sceneUpdater()->entityCount(), 4U);
  EXPECT_FALSE(
      model.sceneUpdater()->cloud("/LIDAR_TOP")->drawable->isVisible());
  for (int i = 0; i < 2; ++i) {
    EXPECT_TRUE(model.engine().frame());
  }

  // Extra camera windows come from the Windows menu.
  model.addCameraView("/CAM_BACK/image_rect_compressed");
  EXPECT_EQ(model.videos().size(), 3U);
  EXPECT_NE(model.videos()[1].title, model.videos()[2].title);
  EXPECT_TRUE(model.engine().frame());
  model.videos()[2].open = false;
  model.pruneClosedWindows();
  EXPECT_EQ(model.videos().size(), 2U);

  // Opening again replaces the replay; tearing down releases everything.
  const size_t prePasses = model.engine().getRenderer()->prePassCount();
  ASSERT_TRUE(model.openRecording(fixture));
  EXPECT_EQ(model.engine().getRenderer()->prePassCount(), prePasses);
  EXPECT_TRUE(model.engine().frame());
  model.teardown();
  EXPECT_EQ(model.recording(), nullptr);
  EXPECT_EQ(model.sceneUpdater(), nullptr);
  EXPECT_TRUE(model.videos().empty());
  EXPECT_EQ(model.engine().getRenderer()->drawableCount(), 0U);
  EXPECT_EQ(model.engine().getRenderer()->prePassCount(), 0U);
  model.engine().cleanUp();
}

}  // namespace camelot

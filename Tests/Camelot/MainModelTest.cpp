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

#include "Avalon/src/ui/TreeWidget.h"
#include "Camelot/src/main/MainModel.h"

namespace camelot {

// The application's demo scene and UI run through the engine on the mock ICD.
TEST(MainModelTest, DemoSceneAndUiRunForAFewFrames) {
  MainModel model;
  model.engine().init();
  model.populateDemoScene();
  EXPECT_EQ(model.objects().size(), 5U);
  ASSERT_NE(model.points(), nullptr);
  EXPECT_EQ(avalon::TreeWidget::countNodes(model.sceneTree()), 8U);

  model.setupUi();
  ASSERT_NE(model.video(), nullptr);
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(model.engine().frame());
  }
  EXPECT_GE(model.frameTimeGraph().size(), 3U);
  EXPECT_GE(model.video()->framesShown(), 1U);

  // Toggling a tree node hides the object.
  avalon::TreeNode* objects = model.sceneTree().find("Objects");
  ASSERT_NE(objects, nullptr);
  avalon::TreeWidget::setVisible(*objects->find("red box"), false);
  EXPECT_FALSE(model.objects().front()->isVisible());
  EXPECT_TRUE(model.engine().frame());

  model.teardown();
  model.teardown();  // idempotent
  model.engine().cleanUp();
}

}  // namespace camelot

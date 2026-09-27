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

#include <vector>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/ui/TreeWidget.h"

namespace avalon {

namespace {
TreeNode makeTree(std::vector<bool>* toggles) {
  TreeNode root{.label = "root"};
  TreeNode& group = root.add("group");
  group.add("a", true, [toggles](bool v) { toggles->push_back(v); });
  group.add("b", false, [toggles](bool v) { toggles->push_back(v); });
  root.add("c");
  return root;
}
}  // namespace

TEST(TreeWidgetTest, BuildsAndCountsNodes) {
  std::vector<bool> toggles;
  TreeNode root = makeTree(&toggles);
  EXPECT_EQ(TreeWidget::countNodes(root), 5U);
  EXPECT_EQ(TreeWidget::countVisibleLeaves(root), 2U);  // a and c
  ASSERT_NE(root.find("group"), nullptr);
  EXPECT_EQ(root.find("group")->find("b")->visible, false);
  EXPECT_EQ(root.find("missing"), nullptr);
}

TEST(TreeWidgetTest, SetVisibleCascadesAndFiresCallbacks) {
  std::vector<bool> toggles;
  TreeNode root = makeTree(&toggles);
  TreeWidget::setVisible(*root.find("group"), false);
  EXPECT_EQ(TreeWidget::countVisibleLeaves(root), 1U);  // only c
  EXPECT_EQ(toggles, (std::vector<bool>{false, false}));
  TreeWidget::setVisible(root, true);
  EXPECT_EQ(TreeWidget::countVisibleLeaves(root), 3U);
  EXPECT_EQ(toggles.size(), 4U);
}

TEST(TreeWidgetTest, DrawsInsideAFrame) {
  Avalon avalon;
  avalon.init();
  std::vector<bool> toggles;
  TreeNode root = makeTree(&toggles);
  bool changed = true;
  avalon.setUiCallback([&]() {
    ImGui::Begin("tree");
    changed = TreeWidget::draw(root);
    ImGui::End();
  });
  EXPECT_TRUE(avalon.frame());
  EXPECT_FALSE(changed);  // nobody clicked
  EXPECT_TRUE(toggles.empty());
  avalon.cleanUp();
}

}  // namespace avalon

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

#ifndef AVALON_SRC_UI_TREEWIDGET_H_
#define AVALON_SRC_UI_TREEWIDGET_H_

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace avalon {

// One row of a TreeWidget: a label, a visibility checkbox and children.
// onToggle fires when the checkbox changes (also through setVisible()).
struct TreeNode {
  std::string label;
  bool visible{true};
  std::vector<TreeNode> children;
  std::function<void(bool visible)> onToggle;

  // Appends a child and returns it. The reference is invalidated by the
  // next add() on the same node (the children live in a std::vector).
  TreeNode& add(std::string childLabel, bool childVisible = true,
                std::function<void(bool)> toggle = nullptr);
  // Finds a direct child by label, or nullptr.
  [[nodiscard]] TreeNode* find(const std::string& childLabel);
};

// Draws a TreeNode hierarchy with ImGui tree nodes and checkboxes.
class TreeWidget {
 public:
  // Draws `root` and its children. Returns true if any checkbox changed.
  static bool draw(TreeNode& root);
  // Sets visibility on the node and every descendant, firing onToggle.
  static void setVisible(TreeNode& node, bool visible);
  [[nodiscard]] static size_t countNodes(const TreeNode& node);
  [[nodiscard]] static size_t countVisibleLeaves(const TreeNode& node);
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_TREEWIDGET_H_

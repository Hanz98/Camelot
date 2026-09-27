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

#include "Avalon/src/ui/TreeWidget.h"

#include <imgui.h>

#include <string>
#include <utility>

namespace avalon {

TreeNode& TreeNode::add(std::string childLabel, bool childVisible,
                        std::function<void(bool)> toggle) {
  children.push_back(TreeNode{.label = std::move(childLabel),
                              .visible = childVisible,
                              .children = {},
                              .onToggle = std::move(toggle)});
  return children.back();
}

TreeNode* TreeNode::find(const std::string& childLabel) {
  for (TreeNode& child : children) {
    if (child.label == childLabel) {
      return &child;
    }
  }
  return nullptr;
}

namespace {

void fireToggle(TreeNode& node) {
  if (node.onToggle) {
    node.onToggle(node.visible);
  }
}

bool drawNode(TreeNode& node) {
  bool changed = false;
  ImGui::PushID(&node);
  if (ImGui::Checkbox("##visible", &node.visible)) {
    TreeWidget::setVisible(node, node.visible);
    changed = true;
  }
  ImGui::SameLine();
  if (node.children.empty()) {
    ImGui::TreeNodeEx(node.label.c_str(),
                      ImGuiTreeNodeFlags_Leaf |
                          ImGuiTreeNodeFlags_NoTreePushOnOpen |
                          ImGuiTreeNodeFlags_SpanAvailWidth);
  } else if (ImGui::TreeNodeEx(node.label.c_str(),
                               ImGuiTreeNodeFlags_DefaultOpen |
                                   ImGuiTreeNodeFlags_SpanAvailWidth)) {
    for (TreeNode& child : node.children) {
      changed = drawNode(child) || changed;
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
  return changed;
}

}  // namespace

bool TreeWidget::draw(TreeNode& root) { return drawNode(root); }

void TreeWidget::setVisible(TreeNode& node, bool visible) {
  node.visible = visible;
  fireToggle(node);
  for (TreeNode& child : node.children) {
    setVisible(child, visible);
  }
}

size_t TreeWidget::countNodes(const TreeNode& node) {
  size_t count = 1;
  for (const TreeNode& child : node.children) {
    count += countNodes(child);
  }
  return count;
}

size_t TreeWidget::countVisibleLeaves(const TreeNode& node) {
  if (node.children.empty()) {
    return node.visible ? 1 : 0;
  }
  size_t count = 0;
  for (const TreeNode& child : node.children) {
    count += countVisibleLeaves(child);
  }
  return count;
}

}  // namespace avalon

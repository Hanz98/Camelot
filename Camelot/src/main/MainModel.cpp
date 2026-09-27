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

#include "Camelot/src/main/MainModel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/geometry/Shapes.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/TestPatternSource.h"

namespace camelot {

namespace {
constexpr uint32_t kVideoWidth = 320;
constexpr uint32_t kVideoHeight = 180;
constexpr double kVideoFps = 25.0;
constexpr const char* kDockspaceName = "CamelotDockspace";
constexpr const char* kSceneWindow = "Scene";
}  // namespace

void MainModel::test() {
  m_avalon.init();
  m_avalon.test();
  m_avalon.cleanUp();
}

void MainModel::run() {
  m_avalon.init();
  populateDemoScene();
  setupUi();
  while (m_avalon.frame()) {
    pruneClosedWindows();
  }
  teardown();
  m_avalon.cleanUp();
}

void MainModel::teardown() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  m_avalon.setUiCallback(nullptr);
  if (renderer != nullptr) {
    for (VideoWindow& video : m_videos) {
      renderer->removePrePass(video.texture);
    }
    for (SceneViewWindow& view : m_sceneViews) {
      renderer->removePrePass(view.view);
    }
    renderer->setDrawablesInMainPass(true);
  }
  m_videos.clear();
  m_sceneViews.clear();
  m_graphs.clear();
  m_sceneTree = avalon::TreeNode{};
  m_points.reset();
  m_objects.clear();
  m_layoutBuilt = false;
}

void MainModel::populateDemoScene() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  const std::shared_ptr<avalon::Device> device = m_avalon.getDevice();
  const std::shared_ptr<avalon::VmaAllocatorWrapper> allocator =
      m_avalon.getAllocator();

  m_sceneTree = avalon::TreeNode{.label = "Scene"};
  avalon::TreeNode& objects = m_sceneTree.add("Objects");

  auto add = [&](const std::string& name, const avalon::MeshData& mesh,
                 const glm::vec3& position, const glm::vec4& color) {
    auto drawable =
        std::make_shared<avalon::MeshDrawable>(device, allocator, mesh);
    drawable->setTransform(glm::translate(glm::mat4(1.0F), position));
    drawable->setColor(color);
    renderer->addDrawable(drawable);
    m_objects.push_back(drawable);
    objects.add(name, true,
                [drawable](bool visible) { drawable->setVisible(visible); });
  };

  add("red box", avalon::shapes::box(glm::vec3(1.0F)), {0.0F, 0.0F, 0.5F},
      {0.9F, 0.3F, 0.3F, 1.0F});
  add("green sphere", avalon::shapes::sphere(0.5F), {2.0F, 0.0F, 0.5F},
      {0.3F, 0.9F, 0.3F, 1.0F});
  add("blue pillar", avalon::shapes::box(glm::vec3(0.6F, 0.6F, 2.0F)),
      {0.0F, 2.0F, 1.0F}, {0.3F, 0.4F, 0.9F, 1.0F});
  add("yellow cylinder", avalon::shapes::cylinder(0.4F, 1.2F),
      {-2.0F, -1.0F, 0.6F}, {0.9F, 0.8F, 0.2F, 1.0F});
  // A thin slab as a stand-in ground plane until the grid (T6) lands.
  add("ground", avalon::shapes::box(glm::vec3(8.0F, 8.0F, 0.02F)),
      {0.0F, 0.0F, -0.01F}, {0.35F, 0.35F, 0.38F, 1.0F});

  // A single point above the scene: the seed of the point-cloud feature.
  // Added to the root last: TreeNode::add() may reallocate the children, which
  // would invalidate the `objects` reference used above.
  m_points = std::make_shared<avalon::PointCloudDrawable>(device, allocator);
  m_points->setPoint({1.0F, -1.5F, 1.5F}, {1.0F, 1.0F, 1.0F, 1.0F});
  m_points->setPointSize(12.0F);
  renderer->addDrawable(m_points);
  m_sceneTree.add("Points", true, [points = m_points](bool visible) {
    points->setVisible(visible);
  });
}

MainModel::SceneViewWindow& MainModel::addSceneView() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  SceneViewWindow window;
  window.title = fmt::format("3D view {}", m_nextViewId++);
  window.view = std::make_shared<avalon::SceneViewWidget>(
      window.title, m_avalon.getDevice(), m_avalon.getAllocator(),
      m_avalon.getUi(), renderer, avalon::Renderer::kFramesInFlight);
  renderer->addPrePass(window.view);
  m_sceneViews.push_back(std::move(window));
  return m_sceneViews.back();
}

MainModel::GraphWindow& MainModel::addGraph() {
  GraphWindow window;
  window.title = fmt::format("Frame time {}", m_nextGraphId++);
  window.graph = std::make_unique<avalon::GraphWidget>("frame time", 300, "ms");
  m_graphs.push_back(std::move(window));
  return m_graphs.back();
}

MainModel::VideoWindow& MainModel::addVideo() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  const std::shared_ptr<avalon::Device> device = m_avalon.getDevice();
  const std::shared_ptr<avalon::VmaAllocatorWrapper> allocator =
      m_avalon.getAllocator();
  VideoWindow window;
  window.title = fmt::format("Video {}", m_nextVideoId++);
  // Test video with a GPU overlay: a box outline that follows the sweeping
  // bar and a marker point at its centre, both drawn onto the frame on the
  // GPU before the UI samples it.
  window.texture = std::make_shared<avalon::VideoTexture>(
      device, allocator, m_avalon.getUi(), kVideoWidth, kVideoHeight,
      avalon::Renderer::kFramesInFlight);
  renderer->addPrePass(window.texture);
  auto source = std::make_shared<avalon::TestPatternSource>(
      kVideoWidth, kVideoHeight, kVideoFps);
  window.widget = std::make_unique<avalon::VideoWidget>("test pattern", source,
                                                        window.texture);
  window.box = std::make_shared<avalon::MeshDrawable>(
      device, allocator, avalon::shapes::box(glm::vec3(1.0F)));
  window.box->setColor({0.1F, 0.9F, 0.9F, 0.35F});
  window.texture->addOverlay(window.box);
  window.marker =
      std::make_shared<avalon::PointCloudDrawable>(device, allocator);
  window.marker->setPointSize(10.0F);
  window.texture->addOverlay(window.marker);
  m_videos.push_back(std::move(window));
  return m_videos.back();
}

void MainModel::pruneClosedWindows() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  for (auto it = m_sceneViews.begin(); it != m_sceneViews.end();) {
    if (!it->open) {
      renderer->removePrePass(it->view);
      it = m_sceneViews.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = m_videos.begin(); it != m_videos.end();) {
    if (!it->open) {
      renderer->removePrePass(it->texture);
      it = m_videos.erase(it);
    } else {
      ++it;
    }
  }
  std::erase_if(m_graphs, [](const GraphWindow& g) { return !g.open; });
}

void MainModel::setupUi() {
  m_avalon.getRenderer()->setDrawablesInMainPass(false);
  addSceneView();
  addGraph();
  addVideo();
  m_avalon.setUiCallback([this]() { buildUi(); });
}

void MainModel::buildDefaultLayout() {
  // First frame only: split the dockspace into the scene tree on the left,
  // the 3D view in the middle, the graph below it and the video on the right.
  const ImGuiID dockspace = ImGui::GetID(kDockspaceName);
  if (ImGui::DockBuilderGetNode(dockspace) == nullptr ||
      ImGui::DockBuilderGetNode(dockspace)->IsLeafNode()) {
    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace,
                                  ImGui::GetMainViewport()->WorkSize);
    ImGuiID centre = dockspace;
    const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left,
                                                     0.22F, nullptr, &centre);
    const ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right,
                                                      0.30F, nullptr, &centre);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down,
                                                       0.28F, nullptr, &centre);
    ImGui::DockBuilderDockWindow(kSceneWindow, left);
    if (!m_sceneViews.empty()) {
      ImGui::DockBuilderDockWindow(m_sceneViews.front().title.c_str(), centre);
    }
    if (!m_graphs.empty()) {
      ImGui::DockBuilderDockWindow(m_graphs.front().title.c_str(), bottom);
    }
    if (!m_videos.empty()) {
      ImGui::DockBuilderDockWindow(m_videos.front().title.c_str(), right);
    }
    ImGui::DockBuilderFinish(dockspace);
  }
  m_layoutBuilt = true;
}

void MainModel::drawMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      if (ImGui::MenuItem("New 3D view")) {
        addSceneView();
      }
      if (ImGui::MenuItem("New graph")) {
        addGraph();
      }
      if (ImGui::MenuItem("New video")) {
        addVideo();
      }
      ImGui::Separator();
      ImGui::MenuItem(kSceneWindow, nullptr, &m_sceneWindowOpen);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void MainModel::drawSceneWindow() {
  if (!m_sceneWindowOpen) {
    return;
  }
  if (ImGui::Begin(kSceneWindow, &m_sceneWindowOpen)) {
    avalon::TreeWidget::draw(m_sceneTree);
    ImGui::Separator();
    ImGui::TextUnformatted(fmt::format("{} views, {} graphs, {} videos",
                                       m_sceneViews.size(), m_graphs.size(),
                                       m_videos.size())
                               .c_str());
  }
  ImGui::End();
}

void MainModel::updateVideoOverlays(double dt) {
  for (VideoWindow& video : m_videos) {
    video.widget->tick(dt);
    // Move the overlay with the bar of the test pattern (pixel space).
    const auto* pattern = dynamic_cast<const avalon::TestPatternSource*>(
        video.widget->source().get());
    const float barX =
        pattern != nullptr
            ? static_cast<float>(pattern->barX(
                  pattern->frameIndex() == 0 ? 0 : pattern->frameIndex() - 1))
            : 0.0F;
    const float centreY =
        static_cast<float>(kVideoHeight) * 0.5F +
        static_cast<float>(std::sin(m_time * 2.0 * std::numbers::pi * 0.5) *
                           30.0);
    glm::mat4 box = glm::translate(glm::mat4(1.0F), {barX, centreY, 0.0F});
    box = glm::scale(box, {40.0F, 60.0F, 1.0F});
    video.box->setTransform(box);
    video.marker->setPoint({barX, centreY, 0.0F}, {1.0F, 0.2F, 0.2F, 1.0F});
  }
}

void MainModel::buildUi() {
  const double dt = m_avalon.getLastFrameSeconds();
  m_time += dt;
  for (GraphWindow& graph : m_graphs) {
    graph.graph->push(static_cast<float>(dt * 1000.0));
  }
  updateVideoOverlays(dt);

  drawMenuBar();
  ImGui::DockSpaceOverViewport(ImGui::GetID(kDockspaceName),
                               ImGui::GetMainViewport());
  if (!m_layoutBuilt) {
    buildDefaultLayout();
  }

  drawSceneWindow();
  for (SceneViewWindow& view : m_sceneViews) {
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(view.title.c_str(), &view.open)) {
      view.view->draw();
    }
    ImGui::End();
  }
  for (GraphWindow& graph : m_graphs) {
    ImGui::SetNextWindowSize(ImVec2(360, 220), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(graph.title.c_str(), &graph.open)) {
      graph.graph->draw();
    }
    ImGui::End();
  }
  for (VideoWindow& video : m_videos) {
    ImGui::SetNextWindowSize(ImVec2(380, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(video.title.c_str(), &video.open)) {
      video.widget->draw();
    }
    ImGui::End();
  }
}

}  // namespace camelot

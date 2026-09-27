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

#ifndef CAMELOT_SRC_MAIN_MAINMODEL_H_
#define CAMELOT_SRC_MAIN_MAINMODEL_H_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/PointCloudDrawable.h"
#include "Avalon/src/ui/GraphWidget.h"
#include "Avalon/src/ui/SceneViewWidget.h"
#include "Avalon/src/ui/TreeWidget.h"
#include "Avalon/src/ui/VideoWidget.h"
#include "Camelot/API/main/ICamelot.h"

namespace camelot {

// The application: a scene of drawables and a dockable UI in which every
// widget lives in its own window and can be opened any number of times. The
// 3D scene is one such widget (SceneViewWidget) rendered offscreen; the
// swapchain only carries the UI.
class MainModel : public ICamelot {
 public:
  struct SceneViewWindow {
    std::string title;
    std::shared_ptr<avalon::SceneViewWidget> view;
    bool open{true};
  };
  struct GraphWindow {
    std::string title;
    std::unique_ptr<avalon::GraphWidget> graph;
    bool open{true};
  };
  struct VideoWindow {
    std::string title;
    std::shared_ptr<avalon::VideoTexture> texture;
    std::unique_ptr<avalon::VideoWidget> widget;
    std::shared_ptr<avalon::MeshDrawable> box;
    std::shared_ptr<avalon::PointCloudDrawable> marker;
    bool open{true};
  };

 private:
  avalon::Avalon m_avalon;
  std::vector<std::shared_ptr<avalon::MeshDrawable>> m_objects;
  std::shared_ptr<avalon::PointCloudDrawable> m_points;

  // UI state.
  avalon::TreeNode m_sceneTree;
  bool m_sceneWindowOpen{true};
  std::vector<SceneViewWindow> m_sceneViews;
  std::vector<GraphWindow> m_graphs;
  std::vector<VideoWindow> m_videos;
  unsigned m_nextViewId{1};
  unsigned m_nextGraphId{1};
  unsigned m_nextVideoId{1};
  bool m_layoutBuilt{false};
  double m_time{0.0};

 public:
  MainModel() = default;
  void test();

  // Initialises the engine and runs the frame loop until the window closes.
  void run();

  // Fills the scene with a few boxes, spheres, a cylinder and a single point
  // until real data (roadmap T7/T8) replaces them. Requires an initialised
  // engine.
  void populateDemoScene();
  // Creates the default windows: one 3D view, the scene tree, a frame-time
  // graph and a test video. Requires an initialised engine.
  void setupUi();
  // Builds the ImGui windows for one frame (called from the engine).
  void buildUi();
  // Releases UI and scene resources; safe to call more than once.
  void teardown();

  // Windows can be added any number of times (also from the Windows menu).
  SceneViewWindow& addSceneView();
  GraphWindow& addGraph();
  VideoWindow& addVideo();
  // Drops the windows whose close button was pressed (or `open` cleared).
  void pruneClosedWindows();

  [[nodiscard]] avalon::Avalon& engine() { return m_avalon; }
  [[nodiscard]] const std::vector<std::shared_ptr<avalon::MeshDrawable>>&
  objects() const {
    return m_objects;
  }
  [[nodiscard]] std::shared_ptr<avalon::PointCloudDrawable> points() const {
    return m_points;
  }
  [[nodiscard]] avalon::TreeNode& sceneTree() { return m_sceneTree; }
  [[nodiscard]] std::vector<SceneViewWindow>& sceneViews() {
    return m_sceneViews;
  }
  [[nodiscard]] std::vector<GraphWindow>& graphs() { return m_graphs; }
  [[nodiscard]] std::vector<VideoWindow>& videos() { return m_videos; }

 private:
  void buildDefaultLayout();
  void drawMenuBar();
  void drawSceneWindow();
  void updateVideoOverlays(double dt);
};

}  // namespace camelot

#endif  // CAMELOT_SRC_MAIN_MAINMODEL_H_

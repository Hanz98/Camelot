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
#include <filesystem>  // NOLINT(build/c++17)
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/PointCloudDrawable.h"
#include "Avalon/src/ui/GraphWidget.h"
#include "Avalon/src/ui/SceneViewWidget.h"
#include "Avalon/src/ui/TreeWidget.h"
#include "Avalon/src/ui/VideoWidget.h"
#include "Camelot/API/main/ICamelot.h"
#include "Camelot/src/replay/Recording.h"
#include "Camelot/src/replay/SceneUpdater.h"

namespace camelot {

// The application: a scene of drawables and a dockable UI in which every
// widget lives in its own window and can be opened any number of times. The
// 3D scene is one such widget (SceneViewWidget) rendered offscreen; the
// swapchain only carries the UI. Without a recording the scene is the demo
// scene; openRecording() replaces it with the replay of an MCAP file.
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
  // Either a test video (widget, texture and overlays owned here) or a
  // camera window of a recording (`topic` set, widget null: the texture is
  // the SceneUpdater's and is looked up every frame).
  struct VideoWindow {
    std::string title;
    std::string topic;
    std::shared_ptr<avalon::VideoTexture> texture;
    std::unique_ptr<avalon::VideoWidget> widget;
    std::shared_ptr<avalon::MeshDrawable> box;
    std::shared_ptr<avalon::PointCloudDrawable> marker;
    bool open{true};

    [[nodiscard]] bool isCamera() const { return !topic.empty(); }
  };

  static constexpr const char* kTimelineWindow = "Timeline";
  static constexpr const char* kTopicsBranch = "Topics";
  static constexpr const char* kUnsupportedBranch = "Unsupported topics";

 private:
  avalon::Avalon m_avalon;
  std::vector<std::shared_ptr<avalon::MeshDrawable>> m_objects;
  std::shared_ptr<avalon::PointCloudDrawable> m_points;
  std::unique_ptr<Recording> m_recording;
  std::unique_ptr<SceneUpdater> m_sceneUpdater;

  // UI state.
  avalon::TreeNode m_sceneTree;
  avalon::TreeNode m_unsupportedTree;
  bool m_sceneWindowOpen{true};
  bool m_timelineOpen{true};
  std::string m_openError;
  std::vector<SceneViewWindow> m_sceneViews;
  std::vector<GraphWindow> m_graphs;
  std::vector<VideoWindow> m_videos;
  unsigned m_nextViewId{1};
  unsigned m_nextGraphId{1};
  unsigned m_nextVideoId{1};
  bool m_layoutBuilt{false};
  std::string m_egoFrame;
  bool m_followEgo{true};
  double m_time{0.0};

 public:
  MainModel() = default;
  void test();

  // Initialises the engine and runs the frame loop until the window closes.
  // With a recording path the scene is the replay, otherwise the demo scene.
  void run(const std::filesystem::path& recording = {});

  // Fills the scene with a few boxes, spheres, a cylinder and a single point
  // until real data (roadmap T7/T8) replaces them. Requires an initialised
  // engine.
  void populateDemoScene();
  // Replaces the scene with the replay of an MCAP file: topic tree, camera
  // windows, timeline. Errors are logged, shown in a modal and reported as
  // false; the previous scene is kept then. Requires an initialised engine.
  bool openRecording(const std::filesystem::path& path);
  // Creates the default windows: one 3D view, the scene tree, a frame-time
  // graph and a test video (only without a recording). Requires an
  // initialised engine.
  void setupUi();
  // Builds the ImGui windows for one frame (called from the engine).
  void buildUi();
  // Releases UI and scene resources; safe to call more than once.
  void teardown();

  // Windows can be added any number of times (also from the Windows menu).
  SceneViewWindow& addSceneView();
  GraphWindow& addGraph();
  VideoWindow& addVideo();
  // A window showing the camera of an image topic of the open recording.
  VideoWindow& addCameraView(const std::string& topic);
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
  [[nodiscard]] Recording* recording() const { return m_recording.get(); }
  [[nodiscard]] SceneUpdater* sceneUpdater() const {
    return m_sceneUpdater.get();
  }
  [[nodiscard]] const std::string& openError() const { return m_openError; }
  [[nodiscard]] avalon::TreeNode& sceneTree() { return m_sceneTree; }
  [[nodiscard]] avalon::TreeNode& unsupportedTree() {
    return m_unsupportedTree;
  }
  [[nodiscard]] std::vector<SceneViewWindow>& sceneViews() {
    return m_sceneViews;
  }
  [[nodiscard]] std::vector<GraphWindow>& graphs() { return m_graphs; }
  [[nodiscard]] std::vector<VideoWindow>& videos() { return m_videos; }
  // The image topics of the open recording (empty without one).
  [[nodiscard]] std::vector<std::string> cameraTopics() const;
  // The frame the 3D views follow while a recording plays: the render frame's
  // first child ("base_link" when present), i.e. the recorded vehicle. Empty
  // without a recording or without transforms.
  [[nodiscard]] const std::string& egoFrame() const { return m_egoFrame; }
  // Position of the ego frame in the render frame at the playback time.
  [[nodiscard]] std::optional<glm::vec3> egoPosition() const;
  // Whether every 3D view keeps its orbit target on the ego frame.
  [[nodiscard]] bool followEgo() const { return m_followEgo; }
  void setFollowEgo(bool follow) { m_followEgo = follow; }

 private:
  void clearScene();
  void buildTopicTree();
  void buildDefaultLayout();
  void drawMenuBar();
  void drawSceneWindow();
  void drawTimelineWindow();
  void drawCameraWindow(const VideoWindow& video);
  void drawOpenErrorModal();
  void updateVideoOverlays(double dt);
  void chooseEgoFrame();
  // Points a view at the ego frame from a fixed distance above and behind it.
  void frameOnEgo(avalon::SceneViewWidget& view) const;
  void followEgoVehicle();
};

}  // namespace camelot

#endif  // CAMELOT_SRC_MAIN_MAINMODEL_H_

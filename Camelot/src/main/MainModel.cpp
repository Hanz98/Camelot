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

#include <cmath>
#include <memory>
#include <numbers>
#include <string>

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
  }
  teardown();
  m_avalon.cleanUp();
}

void MainModel::teardown() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  if (renderer != nullptr && m_videoTexture != nullptr) {
    renderer->removePrePass(m_videoTexture);
  }
  m_avalon.setUiCallback(nullptr);
  m_video.reset();
  m_videoBox.reset();
  m_videoMarker.reset();
  m_videoTexture.reset();
  m_sceneTree = avalon::TreeNode{};
  m_points.reset();
  m_objects.clear();
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

void MainModel::setupUi() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  const std::shared_ptr<avalon::Device> device = m_avalon.getDevice();
  const std::shared_ptr<avalon::VmaAllocatorWrapper> allocator =
      m_avalon.getAllocator();

  // Test video with a GPU overlay: a box outline that follows the sweeping
  // bar and a marker point at its centre, both drawn onto the frame on the
  // GPU before the UI samples it.
  m_videoTexture = std::make_shared<avalon::VideoTexture>(
      device, allocator, m_avalon.getUi(), kVideoWidth, kVideoHeight,
      avalon::Renderer::kFramesInFlight);
  renderer->addPrePass(m_videoTexture);
  auto source = std::make_shared<avalon::TestPatternSource>(
      kVideoWidth, kVideoHeight, kVideoFps);
  m_video = std::make_unique<avalon::VideoWidget>("test pattern", source,
                                                  m_videoTexture);

  m_videoBox = std::make_shared<avalon::MeshDrawable>(
      device, allocator, avalon::shapes::box(glm::vec3(1.0F)));
  m_videoBox->setColor({0.1F, 0.9F, 0.9F, 0.35F});
  m_videoTexture->addOverlay(m_videoBox);
  m_videoMarker =
      std::make_shared<avalon::PointCloudDrawable>(device, allocator);
  m_videoMarker->setPointSize(10.0F);
  m_videoTexture->addOverlay(m_videoMarker);

  m_avalon.setUiCallback([this]() { buildUi(); });
}

void MainModel::buildUi() {
  const double dt = m_avalon.getLastFrameSeconds();
  m_time += dt;
  m_frameTimeGraph.push(static_cast<float>(dt * 1000.0));

  if (m_video) {
    m_video->tick(dt);
    // Move the overlay with the bar of the test pattern (pixel space).
    const auto* pattern =
        dynamic_cast<const avalon::TestPatternSource*>(m_video->source().get());
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
    m_videoBox->setTransform(box);
    m_videoMarker->setPoint({barX, centreY, 0.0F}, {1.0F, 0.2F, 0.2F, 1.0F});
  }

  ImGui::SetNextWindowSize(ImVec2(280, 260), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Scene")) {
    avalon::TreeWidget::draw(m_sceneTree);
    ImGui::Separator();
    if (ImGui::Button("Reset camera")) {
      m_avalon.getCamera()->reset();
    }
  }
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(360, 220), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Frame time")) {
    m_frameTimeGraph.draw();
  }
  ImGui::End();

  if (m_video) {
    ImGui::SetNextWindowSize(ImVec2(380, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Video")) {
      m_video->draw();
    }
    ImGui::End();
  }
}

}  // namespace camelot

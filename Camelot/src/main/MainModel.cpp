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

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/geometry/Shapes.h"

namespace camelot {

void MainModel::test() {
  m_avalon.init();
  m_avalon.test();
  m_avalon.cleanUp();
}

void MainModel::run() {
  m_avalon.init();
  populateDemoScene();
  while (m_avalon.frame()) {
  }
  m_points.reset();
  m_objects.clear();
  m_avalon.cleanUp();
}

void MainModel::populateDemoScene() {
  avalon::Renderer* renderer = m_avalon.getRenderer();
  const std::shared_ptr<avalon::Device> device = m_avalon.getDevice();
  const std::shared_ptr<avalon::VmaAllocatorWrapper> allocator =
      m_avalon.getAllocator();

  auto add = [&](const avalon::MeshData& mesh, const glm::vec3& position,
                 const glm::vec4& color) {
    auto drawable =
        std::make_shared<avalon::MeshDrawable>(device, allocator, mesh);
    drawable->setTransform(glm::translate(glm::mat4(1.0F), position));
    drawable->setColor(color);
    renderer->addDrawable(drawable);
    m_objects.push_back(drawable);
  };

  add(avalon::shapes::box(glm::vec3(1.0F)), {0.0F, 0.0F, 0.5F},
      {0.9F, 0.3F, 0.3F, 1.0F});
  add(avalon::shapes::sphere(0.5F), {2.0F, 0.0F, 0.5F},
      {0.3F, 0.9F, 0.3F, 1.0F});
  add(avalon::shapes::box(glm::vec3(0.6F, 0.6F, 2.0F)), {0.0F, 2.0F, 1.0F},
      {0.3F, 0.4F, 0.9F, 1.0F});
  add(avalon::shapes::cylinder(0.4F, 1.2F), {-2.0F, -1.0F, 0.6F},
      {0.9F, 0.8F, 0.2F, 1.0F});
  // A single point above the scene: the seed of the point-cloud feature.
  m_points = std::make_shared<avalon::PointCloudDrawable>(device, allocator);
  m_points->setPoint({1.0F, -1.5F, 1.5F}, {1.0F, 1.0F, 1.0F, 1.0F});
  m_points->setPointSize(12.0F);
  renderer->addDrawable(m_points);

  // A thin slab as a stand-in ground plane until the grid (T6) lands.
  add(avalon::shapes::box(glm::vec3(8.0F, 8.0F, 0.02F)), {0.0F, 0.0F, -0.01F},
      {0.35F, 0.35F, 0.38F, 1.0F});
}

}  // namespace camelot

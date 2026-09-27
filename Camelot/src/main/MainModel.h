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

#include <memory>
#include <vector>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/PointCloudDrawable.h"
#include "Camelot/API/main/ICamelot.h"

namespace camelot {

class MainModel : public ICamelot {
 private:
  avalon::Avalon m_avalon;
  std::vector<std::shared_ptr<avalon::MeshDrawable>> m_objects;
  std::shared_ptr<avalon::PointCloudDrawable> m_points;

 public:
  MainModel() = default;
  void test();

  // Initialises the engine and runs the frame loop until the window closes.
  void run();

  // Fills the scene with a few boxes, spheres, a cylinder and a single point
  // until real data (roadmap T7/T8) replaces them. Requires an initialised
  // engine.
  void populateDemoScene();

  [[nodiscard]] avalon::Avalon& engine() { return m_avalon; }
  [[nodiscard]] const std::vector<std::shared_ptr<avalon::MeshDrawable>>&
  objects() const {
    return m_objects;
  }
  [[nodiscard]] std::shared_ptr<avalon::PointCloudDrawable> points() const {
    return m_points;
  }
};

}  // namespace camelot

#endif  // CAMELOT_SRC_MAIN_MAINMODEL_H_

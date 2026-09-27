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

#ifndef AVALON_SRC_CAMERA_CAMERACONTROLLER_H_
#define AVALON_SRC_CAMERA_CAMERACONTROLLER_H_

#include <functional>
#include <utility>

#include "Avalon/src/camera/Camera.h"
#include "Avalon/src/window/Window.h"

namespace avalon {

// Mouse control for an orbit Camera: left drag orbits, right or middle drag
// pans, the scroll wheel zooms. Feed it events directly or attach() it to a
// Window, which routes the GLFW callbacks here.
class CameraController {
 private:
  Camera* m_camera;  // not owned
  bool m_orbiting{false};
  bool m_panning{false};
  double m_lastX{0.0};
  double m_lastY{0.0};
  float m_orbitSensitivity{0.005F};  // radians per pixel
  float m_panSensitivity{0.0015F};   // fraction of distance per pixel
  float m_zoomStep{1.1F};            // distance factor per scroll notch
  std::function<bool()> m_inputBlocked;

 public:
  explicit CameraController(Camera* camera);

  void attach(Window& window);

  // GLFW-style events: `button` and `action` as in glfwSetMouseButtonCallback.
  void onMouseButton(int button, int action, double x, double y);
  void onCursorMove(double x, double y);
  void onScroll(double deltaY);

  void setOrbitSensitivity(float radiansPerPixel) {
    m_orbitSensitivity = radiansPerPixel;
  }
  void setPanSensitivity(float fractionPerPixel) {
    m_panSensitivity = fractionPerPixel;
  }
  void setZoomStep(float factorPerNotch) { m_zoomStep = factorPerNotch; }
  // While the predicate returns true, button presses and wheel events are
  // ignored (the UI has the mouse). An ongoing drag still ends normally.
  void setInputBlocked(std::function<bool()> predicate) {
    m_inputBlocked = std::move(predicate);
  }
  [[nodiscard]] bool isInputBlocked() const {
    return m_inputBlocked && m_inputBlocked();
  }

  [[nodiscard]] bool isOrbiting() const { return m_orbiting; }
  [[nodiscard]] bool isPanning() const { return m_panning; }
  [[nodiscard]] Camera* camera() const { return m_camera; }
};

}  // namespace avalon

#endif  // AVALON_SRC_CAMERA_CAMERACONTROLLER_H_

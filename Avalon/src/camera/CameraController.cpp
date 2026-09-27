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

#include "Avalon/src/camera/CameraController.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <stdexcept>

namespace avalon {

CameraController::CameraController(Camera* camera) : m_camera(camera) {
  if (m_camera == nullptr) {
    spdlog::error("CameraController: camera is null.");
    throw std::runtime_error("CameraController: camera is null.");
  }
}

void CameraController::attach(Window& window) {
  Window::InputHooks hooks;
  hooks.onMouseButton = [this](int button, int action, int /*mods*/, double x,
                               double y) {
    onMouseButton(button, action, x, y);
  };
  hooks.onCursorMove = [this](double x, double y) { onCursorMove(x, y); };
  hooks.onScroll = [this](double /*dx*/, double dy) { onScroll(dy); };
  window.setInputHooks(hooks);
}

void CameraController::onMouseButton(int button, int action, double x,
                                     double y) {
  const bool pressed = action == GLFW_PRESS;
  if (pressed && isInputBlocked()) {
    return;
  }
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    m_orbiting = pressed;
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT ||
             button == GLFW_MOUSE_BUTTON_MIDDLE) {
    m_panning = pressed;
  }
  m_lastX = x;
  m_lastY = y;
}

void CameraController::onCursorMove(double x, double y) {
  const auto dx = static_cast<float>(x - m_lastX);
  const auto dy = static_cast<float>(y - m_lastY);
  m_lastX = x;
  m_lastY = y;
  if (m_orbiting) {
    // Dragging right turns the scene right (yaw decreases); dragging down
    // tilts the view down (pitch increases).
    m_camera->orbit(-dx * m_orbitSensitivity, dy * m_orbitSensitivity);
  } else if (m_panning) {
    m_camera->pan(-dx * m_panSensitivity, dy * m_panSensitivity);
  }
}

void CameraController::onScroll(double deltaY) {
  if (isInputBlocked()) {
    return;
  }
  // Scrolling up (positive) moves closer.
  m_camera->zoom(std::pow(m_zoomStep, static_cast<float>(-deltaY)));
}

}  // namespace avalon

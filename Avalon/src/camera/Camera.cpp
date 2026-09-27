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

#include "Avalon/src/camera/Camera.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <glm/gtc/matrix_transform.hpp>

namespace avalon {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kPitchLimit = kPi / 2.0F - 0.01F;
constexpr float kDefaultYaw = -kPi / 4.0F;
constexpr float kDefaultPitch = kPi / 6.0F;
constexpr float kDefaultDistance = 6.0F;
constexpr float kDefaultFovY = kPi / 4.0F;
constexpr glm::vec3 kWorldUp{0.0F, 0.0F, 1.0F};
}  // namespace

Camera::Camera() { reset(); }

void Camera::reset() {
  m_target = glm::vec3(0.0F);
  m_yaw = kDefaultYaw;
  m_pitch = kDefaultPitch;
  m_distance = kDefaultDistance;
  m_fovY = kDefaultFovY;
  m_near = 0.1F;
  m_far = 1000.0F;
}

void Camera::setYawPitch(float yaw, float pitch) {
  m_yaw = yaw;
  m_pitch = std::clamp(pitch, -kPitchLimit, kPitchLimit);
}

void Camera::setDistance(float distance) {
  m_distance = std::clamp(distance, kMinDistance, kMaxDistance);
}

void Camera::setPerspective(float fovYRadians, float nearPlane,
                            float farPlane) {
  m_fovY = fovYRadians;
  m_near = nearPlane;
  m_far = farPlane;
}

void Camera::setAspect(float aspect) {
  if (aspect > 0.0F) {
    m_aspect = aspect;
  }
}

void Camera::setAspect(uint32_t width, uint32_t height) {
  if (width > 0 && height > 0) {
    m_aspect = static_cast<float>(width) / static_cast<float>(height);
  }
}

void Camera::orbit(float deltaYaw, float deltaPitch) {
  setYawPitch(m_yaw + deltaYaw, m_pitch + deltaPitch);
}

void Camera::zoom(float factor) {
  if (factor > 0.0F) {
    setDistance(m_distance * factor);
  }
}

void Camera::pan(float deltaRight, float deltaUp) {
  m_target +=
      right() * (deltaRight * m_distance) + up() * (deltaUp * m_distance);
}

glm::vec3 Camera::position() const {
  const float cosPitch = std::cos(m_pitch);
  const glm::vec3 offset(cosPitch * std::cos(m_yaw), cosPitch * std::sin(m_yaw),
                         std::sin(m_pitch));
  return m_target + offset * m_distance;
}

glm::vec3 Camera::forward() const {
  return glm::normalize(m_target - position());
}

glm::vec3 Camera::right() const {
  return glm::normalize(glm::cross(forward(), kWorldUp));
}

glm::vec3 Camera::up() const { return glm::cross(right(), forward()); }

glm::mat4 Camera::view() const {
  return glm::lookAt(position(), m_target, kWorldUp);
}

glm::mat4 Camera::projection() const {
  glm::mat4 proj = glm::perspective(m_fovY, m_aspect, m_near, m_far);
  proj[1][1] *= -1.0F;  // GLM is Y-up in clip space, Vulkan is Y-down.
  return proj;
}

glm::mat4 Camera::viewProjection() const { return projection() * view(); }

CameraUbo Camera::ubo() const {
  CameraUbo ubo;
  ubo.view = view();
  ubo.proj = projection();
  ubo.viewProj = ubo.proj * ubo.view;
  ubo.eye = glm::vec4(position(), 1.0F);
  return ubo;
}

}  // namespace avalon

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

#ifndef AVALON_SRC_CAMERA_CAMERA_H_
#define AVALON_SRC_CAMERA_CAMERA_H_

#include <glm/glm.hpp>

namespace avalon {

// Camera data as the shaders see it (std140: only mat4 and vec4 members).
struct CameraUbo {
  glm::mat4 view{1.0F};
  glm::mat4 proj{1.0F};
  glm::mat4 viewProj{1.0F};
  glm::vec4 eye{0.0F};  // world-space camera position, w unused
};

// Orbit camera in a Z-up right-handed world (robotics convention): the eye
// sits `distance` away from `target` at the given yaw (around Z) and pitch
// (above the XY plane) and always looks at the target.
class Camera {
 private:
  glm::vec3 m_target{0.0F};
  float m_yaw{0.0F};
  float m_pitch{0.0F};
  float m_distance{1.0F};
  float m_fovY{1.0F};
  float m_near{0.1F};
  float m_far{1000.0F};
  float m_aspect{1.0F};

 public:
  static constexpr float kMinDistance = 0.05F;
  static constexpr float kMaxDistance = 10000.0F;

  Camera();

  // Puts the camera back to its initial pose (looking down at the origin
  // from a few metres away).
  void reset();

  void setTarget(const glm::vec3& target) { m_target = target; }
  [[nodiscard]] const glm::vec3& getTarget() const { return m_target; }

  // Pitch is clamped just short of straight up/down to keep lookAt stable.
  void setYawPitch(float yaw, float pitch);
  [[nodiscard]] float getYaw() const { return m_yaw; }
  [[nodiscard]] float getPitch() const { return m_pitch; }

  // Clamped to [kMinDistance, kMaxDistance].
  void setDistance(float distance);
  [[nodiscard]] float getDistance() const { return m_distance; }

  void setPerspective(float fovYRadians, float nearPlane, float farPlane);
  void setAspect(float aspect);
  void setAspect(uint32_t width, uint32_t height);
  [[nodiscard]] float getAspect() const { return m_aspect; }
  [[nodiscard]] float getFovY() const { return m_fovY; }

  // Interactive controls. Deltas are in radians / a multiplicative zoom
  // factor / screen-relative pan amounts scaled by the distance.
  void orbit(float deltaYaw, float deltaPitch);
  void zoom(float factor);
  void pan(float deltaRight, float deltaUp);

  [[nodiscard]] glm::vec3 position() const;
  [[nodiscard]] glm::vec3 forward() const;
  [[nodiscard]] glm::vec3 right() const;
  [[nodiscard]] glm::vec3 up() const;
  [[nodiscard]] glm::mat4 view() const;
  // Vulkan clip space: Y down, depth 0..1.
  [[nodiscard]] glm::mat4 projection() const;
  [[nodiscard]] glm::mat4 viewProjection() const;
  [[nodiscard]] CameraUbo ubo() const;
};

}  // namespace avalon

#endif  // AVALON_SRC_CAMERA_CAMERA_H_

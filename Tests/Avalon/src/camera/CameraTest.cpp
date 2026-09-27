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

#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include <glm/glm.hpp>

#include "Avalon/src/camera/Camera.h"
#include "Avalon/src/camera/CameraController.h"

namespace avalon {

namespace {
constexpr float kEps = 1e-4F;
constexpr float kPi = std::numbers::pi_v<float>;

glm::vec2 project(const Camera& camera, const glm::vec3& point) {
  const glm::vec4 clip = camera.viewProjection() * glm::vec4(point, 1.0F);
  return {clip.x / clip.w, clip.y / clip.w};
}
}  // namespace

TEST(CameraTest, DefaultLooksAtOriginFromAbove) {
  const Camera camera;
  EXPECT_EQ(camera.getTarget(), glm::vec3(0.0F));
  EXPECT_GT(camera.position().z, 0.0F);
  EXPECT_NEAR(glm::length(camera.position() - camera.getTarget()),
              camera.getDistance(), kEps);
  // The target projects to the centre of the screen.
  const glm::vec2 centre = project(camera, camera.getTarget());
  EXPECT_NEAR(centre.x, 0.0F, kEps);
  EXPECT_NEAR(centre.y, 0.0F, kEps);
}

TEST(CameraTest, ProjectionUsesVulkanConventions) {
  Camera camera;
  camera.setAspect(1280, 720);
  const glm::mat4 proj = camera.projection();
  EXPECT_LT(proj[1][1], 0.0F);  // Y flipped for Vulkan clip space
  EXPECT_NEAR(camera.getAspect(), 1280.0F / 720.0F, kEps);
  // A point above the target (world +Z) ends up higher on screen, i.e. at a
  // smaller clip-space y.
  const glm::vec2 up = project(camera, glm::vec3(0.0F, 0.0F, 1.0F));
  EXPECT_LT(up.y, 0.0F);
}

TEST(CameraTest, AspectIgnoresZeroSizes) {
  Camera camera;
  camera.setAspect(2.0F);
  camera.setAspect(0, 100);
  camera.setAspect(0.0F);
  EXPECT_NEAR(camera.getAspect(), 2.0F, kEps);
}

TEST(CameraTest, OrbitChangesYawAndClampsPitch) {
  Camera camera;
  const float yaw = camera.getYaw();
  camera.orbit(0.5F, 0.0F);
  EXPECT_NEAR(camera.getYaw(), yaw + 0.5F, kEps);
  camera.orbit(0.0F, 10.0F);
  EXPECT_LT(camera.getPitch(), kPi / 2.0F);
  camera.orbit(0.0F, -20.0F);
  EXPECT_GT(camera.getPitch(), -kPi / 2.0F);
  // Distance to the target never changes while orbiting.
  EXPECT_NEAR(glm::length(camera.position() - camera.getTarget()),
              camera.getDistance(), kEps);
}

TEST(CameraTest, ZoomIsClamped) {
  Camera camera;
  camera.zoom(1e-9F);
  EXPECT_NEAR(camera.getDistance(), Camera::kMinDistance, kEps);
  camera.zoom(1e12F);
  EXPECT_NEAR(camera.getDistance(), Camera::kMaxDistance, kEps);
  camera.setDistance(3.0F);
  camera.zoom(-1.0F);  // ignored
  camera.zoom(0.0F);   // ignored
  EXPECT_NEAR(camera.getDistance(), 3.0F, kEps);
}

TEST(CameraTest, PanMovesTargetInTheViewPlane) {
  Camera camera;
  const glm::vec3 before = camera.getTarget();
  const glm::vec3 forward = camera.forward();
  camera.pan(1.0F, 0.5F);
  const glm::vec3 delta = camera.getTarget() - before;
  EXPECT_GT(glm::length(delta), 0.0F);
  EXPECT_NEAR(glm::dot(glm::normalize(delta), forward), 0.0F, kEps);
}

TEST(CameraTest, ResetRestoresDefaults) {
  Camera camera;
  const Camera defaults;
  camera.orbit(1.0F, 0.3F);
  camera.zoom(2.0F);
  camera.pan(1.0F, 1.0F);
  camera.reset();
  EXPECT_EQ(camera.getTarget(), defaults.getTarget());
  EXPECT_NEAR(camera.getYaw(), defaults.getYaw(), kEps);
  EXPECT_NEAR(camera.getDistance(), defaults.getDistance(), kEps);
}

TEST(CameraTest, UboMatchesMatrices) {
  Camera camera;
  camera.setAspect(4.0F / 3.0F);
  const CameraUbo ubo = camera.ubo();
  EXPECT_EQ(ubo.view, camera.view());
  EXPECT_EQ(ubo.proj, camera.projection());
  EXPECT_EQ(ubo.viewProj, camera.viewProjection());
  EXPECT_EQ(glm::vec3(ubo.eye), camera.position());
}

TEST(CameraControllerTest, RequiresACamera) {
  EXPECT_THROW(CameraController(nullptr), std::runtime_error);
}

TEST(CameraControllerTest, LeftDragOrbits) {
  Camera camera;
  CameraController controller(&camera);
  const float yaw = camera.getYaw();
  const float pitch = camera.getPitch();
  controller.onMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 100.0, 100.0);
  EXPECT_TRUE(controller.isOrbiting());
  controller.onCursorMove(150.0, 120.0);
  EXPECT_LT(camera.getYaw(), yaw);      // dragged right -> scene turns right
  EXPECT_GT(camera.getPitch(), pitch);  // dragged down -> tilt down
  controller.onMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 150.0, 120.0);
  EXPECT_FALSE(controller.isOrbiting());
  const float yawAfter = camera.getYaw();
  controller.onCursorMove(300.0, 300.0);  // no button: nothing happens
  EXPECT_NEAR(camera.getYaw(), yawAfter, kEps);
}

TEST(CameraControllerTest, RightDragPansAndScrollZooms) {
  Camera camera;
  CameraController controller(&camera);
  const glm::vec3 target = camera.getTarget();
  controller.onMouseButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0.0, 0.0);
  EXPECT_TRUE(controller.isPanning());
  controller.onCursorMove(40.0, 0.0);
  EXPECT_NE(camera.getTarget(), target);
  controller.onMouseButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, 40.0, 0.0);

  const float distance = camera.getDistance();
  controller.onScroll(1.0);  // scroll up: closer
  EXPECT_LT(camera.getDistance(), distance);
  controller.onScroll(-2.0);
  EXPECT_GT(camera.getDistance(), distance);
}

}  // namespace avalon

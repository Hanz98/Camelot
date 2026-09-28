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

#include <gtest/gtest.h>

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/TransformTree.h"

namespace camelot {

namespace {

constexpr float kEpsilon = 1e-4F;
const double kHalfSqrt2 = std::sqrt(0.5);

Vec3 vec3(double x, double y, double z) { return {.x = x, .y = y, .z = z}; }

Quat quat(double x, double y, double z, double w) {
  return {.x = x, .y = y, .z = z, .w = w};
}

Quat identity() { return quat(0.0, 0.0, 0.0, 1.0); }

// A rotation of 90 degrees about z.
Quat quarterTurn() { return quat(0.0, 0.0, kHalfSqrt2, kHalfSqrt2); }

FrameTransform makeTransform(const std::string& parent,
                             const std::string& child, Time t,
                             const Vec3& translation,
                             const Quat& rotation = identity()) {
  FrameTransform tf;
  tf.timestamp = t;
  tf.parentFrameId = parent;
  tf.childFrameId = child;
  tf.translation = translation;
  tf.rotation = rotation;
  return tf;
}

// Fails the test when the lookup found nothing and returns a zero matrix,
// so the checks that follow never touch an empty optional.
glm::mat4 require(const std::optional<glm::mat4>& m) {
  EXPECT_TRUE(m.has_value());
  return m.value_or(glm::mat4(0.0F));
}

glm::vec3 apply(const glm::mat4& m, glm::vec3 p) {
  const glm::vec4 r = m * glm::vec4(p, 1.0F);
  return {r.x, r.y, r.z};
}

void expectNear(glm::vec3 actual, glm::vec3 expected) {
  EXPECT_NEAR(actual.x, expected.x, kEpsilon);
  EXPECT_NEAR(actual.y, expected.y, kEpsilon);
  EXPECT_NEAR(actual.z, expected.z, kEpsilon);
}

}  // namespace

TEST(TransformTreeTest, StaticEdgeMatchesAnyTime) {
  TransformTree tree;
  tree.add(makeTransform("base_link", "LIDAR_TOP", 0, vec3(1.0, 0.0, 2.0)));
  const glm::mat4 m =
      require(tree.lookup("base_link", "LIDAR_TOP", 123'456'789));
  expectNear(apply(m, {0, 0, 0}), {1, 0, 2});
  expectNear(apply(m, {1, 1, 1}), {2, 1, 3});
}

TEST(TransformTreeTest, LookupInTheOtherDirectionIsTheInverse) {
  TransformTree tree;
  tree.add(makeTransform("base_link", "LIDAR_TOP", 0, vec3(1.0, 0.0, 2.0),
                         quarterTurn()));
  const glm::mat4 forward = require(tree.lookup("base_link", "LIDAR_TOP", 0));
  const glm::mat4 backward = require(tree.lookup("LIDAR_TOP", "base_link", 0));
  const glm::vec3 p{0.3F, -0.7F, 1.1F};
  expectNear(apply(backward, apply(forward, p)), p);
  expectNear(apply(backward, {1, 0, 2}), {0, 0, 0});
}

TEST(TransformTreeTest, WalksTheParentChainBothWays) {
  TransformTree tree;
  tree.add(makeTransform("map", "base_link", 0, vec3(10.0, 0.0, 0.0)));
  // The camera is 1 m ahead and rotated 90 degrees about z.
  tree.add(makeTransform("base_link", "CAM_FRONT", 0, vec3(1.0, 0.0, 0.0),
                         quarterTurn()));
  tree.add(makeTransform("base_link", "LIDAR_TOP", 0, vec3(0.0, 0.0, 2.0)));

  // (1,0,0) in the camera -> (0,1,0) rotated, +1 m ahead -> (1,1,0) in
  // base_link -> (11,1,0) in map.
  const glm::mat4 camInMap = require(tree.lookup("map", "CAM_FRONT", 0));
  expectNear(apply(camInMap, {1, 0, 0}), {11, 1, 0});

  const glm::mat4 mapInCam = require(tree.lookup("CAM_FRONT", "map", 0));
  expectNear(apply(mapInCam, {11, 1, 0}), {1, 0, 0});

  // Siblings meet at base_link. The lidar origin is (0,0,2) in base_link =
  // (-1,0,2) relative to the camera origin, rotated by -90 degrees about z
  // -> (0,1,2).
  const glm::mat4 lidarInCam =
      require(tree.lookup("CAM_FRONT", "LIDAR_TOP", 0));
  expectNear(apply(lidarInCam, {0, 0, 0}), {0, 1, 2});
  const glm::mat4 camInLidar =
      require(tree.lookup("LIDAR_TOP", "CAM_FRONT", 0));
  expectNear(apply(camInLidar, {0, 1, 2}), {0, 0, 0});
}

TEST(TransformTreeTest, SelectsTheSampleNearestInTime) {
  TransformTree tree;
  tree.add(makeTransform("map", "base_link", 1000, vec3(10.0, 0.0, 0.0)));
  tree.add(makeTransform("map", "base_link", 0, vec3(0.0, 0.0, 0.0)));
  tree.add(makeTransform("map", "base_link", 2000, vec3(20.0, 0.0, 0.0)));
  EXPECT_EQ(tree.sampleCount("base_link"), 3U);
  const auto x = [&](Time t) {
    return apply(require(tree.lookup("map", "base_link", t)), {0, 0, 0}).x;
  };
  EXPECT_NEAR(x(0), 0.0F, kEpsilon);
  EXPECT_NEAR(x(400), 0.0F, kEpsilon);
  EXPECT_NEAR(x(500), 0.0F, kEpsilon);  // tie -> earlier
  EXPECT_NEAR(x(600), 10.0F, kEpsilon);
  EXPECT_NEAR(x(1000), 10.0F, kEpsilon);
  EXPECT_NEAR(x(1700), 20.0F, kEpsilon);
  EXPECT_NEAR(x(5000), 20.0F, kEpsilon);  // after the last sample
}

TEST(TransformTreeTest, SameTimestampReplacesTheSample) {
  TransformTree tree;
  tree.add(makeTransform("map", "base_link", 5, vec3(1.0, 0.0, 0.0)));
  tree.add(makeTransform("map", "base_link", 5, vec3(2.0, 0.0, 0.0)));
  EXPECT_EQ(tree.sampleCount("base_link"), 1U);
  const glm::mat4 m = require(tree.lookup("map", "base_link", 5));
  EXPECT_NEAR(apply(m, {0, 0, 0}).x, 2.0F, kEpsilon);
}

TEST(TransformTreeTest, ChangingTheParentDropsOldSamples) {
  TransformTree tree;
  tree.add(makeTransform("map", "child", 0, vec3(1.0, 0.0, 0.0)));
  tree.add(makeTransform("map", "child", 10, vec3(2.0, 0.0, 0.0)));
  tree.add(makeTransform("odom", "child", 20, vec3(3.0, 0.0, 0.0)));
  EXPECT_EQ(tree.parent("child").value_or(""), "odom");
  EXPECT_EQ(tree.sampleCount("child"), 1U);
  EXPECT_FALSE(tree.lookup("map", "child", 0).has_value());
  const glm::mat4 m = require(tree.lookup("odom", "child", 0));
  EXPECT_NEAR(apply(m, {0, 0, 0}).x, 3.0F, kEpsilon);
}

TEST(TransformTreeTest, DisconnectedAndUnknownFramesHaveNoTransform) {
  TransformTree tree;
  tree.add(makeTransform("map", "base_link", 0, vec3(1.0, 0.0, 0.0)));
  tree.add(makeTransform("world", "robot", 0, vec3(1.0, 0.0, 0.0)));
  EXPECT_FALSE(tree.lookup("map", "robot", 0).has_value());
  EXPECT_FALSE(tree.lookup("robot", "map", 0).has_value());
  EXPECT_FALSE(tree.lookup("map", "nowhere", 0).has_value());
  EXPECT_FALSE(tree.lookup("nowhere", "base_link", 0).has_value());
  EXPECT_FALSE(tree.lookup("a", "b", 0).has_value());
}

TEST(TransformTreeTest, SameFrameIsIdentity) {
  TransformTree tree;
  const glm::mat4 unknown = require(tree.lookup("map", "map", 0));
  expectNear(apply(unknown, {1, 2, 3}), {1, 2, 3});
  tree.add(makeTransform("map", "base_link", 0, vec3(1.0, 0.0, 0.0)));
  const glm::mat4 same = require(tree.lookup("base_link", "base_link", 0));
  expectNear(apply(same, {1, 2, 3}), {1, 2, 3});
}

TEST(TransformTreeTest, ZeroQuaternionIsIdentityRotation) {
  TransformTree tree;
  tree.add(makeTransform("map", "base_link", 0, vec3(1.0, 2.0, 3.0),
                         quat(0.0, 0.0, 0.0, 0.0)));
  const glm::mat4 m = require(tree.lookup("map", "base_link", 0));
  expectNear(apply(m, {1, 0, 0}), {2, 2, 3});
}

TEST(TransformTreeTest, ListsFramesAndClears) {
  TransformTree tree;
  EXPECT_TRUE(tree.frames().empty());
  tree.add(makeTransform("map", "base_link", 0, vec3(0.0, 0.0, 0.0)));
  tree.add(makeTransform("base_link", "LIDAR_TOP", 0, vec3(0.0, 0.0, 0.0)));
  tree.add(makeTransform("base_link", "CAM_FRONT", 0, vec3(0.0, 0.0, 0.0)));
  const std::vector<std::string> expected{"CAM_FRONT", "LIDAR_TOP", "base_link",
                                          "map"};
  EXPECT_EQ(tree.frames(), expected);
  EXPECT_FALSE(tree.parent("map").has_value());
  EXPECT_EQ(tree.parent("LIDAR_TOP").value_or(""), "base_link");
  EXPECT_EQ(tree.sampleCount("nowhere"), 0U);
  tree.clear();
  EXPECT_TRUE(tree.frames().empty());
  EXPECT_FALSE(tree.lookup("map", "base_link", 0).has_value());
}

TEST(TransformTreeTest, ToMatrixIsTranslateTimesRotate) {
  const TransformTree::Sample sample{
      .time = 0, .translation = vec3(1.0, 0.0, 0.0), .rotation = quarterTurn()};
  const glm::mat4 m = TransformTree::toMatrix(sample);
  // Rotation applies first, then the translation.
  expectNear(apply(m, {1, 0, 0}), {1, 1, 0});
}

}  // namespace camelot

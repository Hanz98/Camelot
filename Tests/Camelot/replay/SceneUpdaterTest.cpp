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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/JpegDecoder.h"
#include "Camelot/src/data/TransformTree.h"
#include "Camelot/src/replay/SceneUpdater.h"

namespace camelot {

namespace {

constexpr Time kSecond = 1'000'000'000ULL;
constexpr uint32_t kWidth = 64;
constexpr uint32_t kHeight = 36;
constexpr const char* kMarkers = "/markers/annotations";
constexpr const char* kImageTopic = "/CAM_FRONT/image_rect_compressed";
constexpr const char* kAnnotationTopic = "/CAM_FRONT/annotations";

Vec3 vec3(double x, double y, double z) { return {.x = x, .y = y, .z = z}; }

Pose poseAt(double x, double y, double z) {
  return Pose{.position = vec3(x, y, z), .orientation = {}};
}

Color rgba(float r, float g, float b, float a) {
  return Color{.r = r, .g = g, .b = b, .a = a};
}

FrameTransform transform(const std::string& parent, const std::string& child,
                         double x, double y, double z) {
  return FrameTransform{.timestamp = 0,
                        .parentFrameId = parent,
                        .childFrameId = child,
                        .translation = vec3(x, y, z),
                        .rotation = {}};
}

SceneEntity cubeEntity(const std::string& id, Time timestamp, double x,
                       Duration lifetime = 0) {
  SceneEntity entity;
  entity.timestamp = timestamp;
  entity.frameId = "base_link";
  entity.id = id;
  entity.lifetime = lifetime;
  entity.cubes.push_back(CubePrimitive{.pose = poseAt(x, 0.0, 0.0),
                                       .size = vec3(2.0, 2.0, 2.0),
                                       .color = rgba(1.0F, 0.0F, 0.0F, 1.0F)});
  entity.texts.push_back(TextPrimitive{.text = "label"});
  return entity;
}

SceneUpdate updateWith(SceneEntity entity) {
  SceneUpdate update;
  update.entities.push_back(std::move(entity));
  return update;
}

LinePrimitive line(LineType type, size_t points) {
  LinePrimitive primitive;
  primitive.type = type;
  for (size_t i = 0; i < points; ++i) {
    primitive.points.push_back(vec3(static_cast<double>(i), 0.0, 0.0));
  }
  primitive.color = rgba(0.0F, 1.0F, 0.0F, 1.0F);
  return primitive;
}

PointCloud cloudOf(const std::string& frameId,
                   const std::vector<glm::vec4>& points) {
  PointCloud cloud;
  cloud.timestamp = kSecond;
  cloud.frameId = frameId;
  cloud.pointStride = sizeof(glm::vec4);
  const std::vector<std::string> names = {"x", "y", "z", "intensity"};
  for (size_t i = 0; i < names.size(); ++i) {
    cloud.fields.push_back({.name = names[i],
                            .offset = static_cast<uint32_t>(i * sizeof(float)),
                            .type = NumericType::kFloat32});
  }
  cloud.data.resize(points.size() * sizeof(glm::vec4));
  std::memcpy(cloud.data.data(), points.data(), cloud.data.size());
  return cloud;
}

CompressedImage jpegImage(uint32_t width, uint32_t height, Time timestamp) {
  std::vector<std::byte> rgba(static_cast<size_t>(width) * height * 4);
  for (size_t i = 0; i < rgba.size(); ++i) {
    rgba[i] = static_cast<std::byte>(i % 251);
  }
  return CompressedImage{
      .timestamp = timestamp,
      .frameId = "CAM_FRONT",
      .data = JpegDecoder::encodeJpegForTests(width, height, rgba),
      .format = "jpeg"};
}

ImageAnnotations boxAndCircle(size_t corners) {
  ImageAnnotations annotations;
  PointsAnnotation loop;
  loop.type = PointsType::kLineLoop;
  for (size_t i = 0; i < corners; ++i) {
    loop.points.push_back({.x = 8.0 + static_cast<double>(i), .y = 8.0});
  }
  loop.outlineColor = rgba(0.0F, 1.0F, 0.0F, 1.0F);
  loop.thickness = 2.0;
  annotations.points.push_back(loop);
  annotations.circles.push_back(CircleAnnotation{
      .position = {.x = 32.0, .y = 18.0}, .diameter = 10.0, .thickness = 1.0});
  annotations.texts.push_back(TextAnnotation{.text = "car"});
  return annotations;
}

}  // namespace

// Drives the updater through the engine on the mock ICD.
class SceneUpdaterTest : public ::testing::Test {
 protected:
  avalon::Avalon m_avalon;
  TransformTree m_tree;
  std::unique_ptr<SceneUpdater> m_updater;
  size_t m_baseDrawables{0};
  size_t m_basePrePasses{0};

  void SetUp() override {
    m_avalon.init();
    m_tree.add(transform("map", "base_link", 1.0, 2.0, 0.0));
    m_tree.add(transform("base_link", "LIDAR_TOP", 0.9, 0.0, 1.8));
    m_updater = std::make_unique<SceneUpdater>(
        m_avalon.getDevice(), m_avalon.getAllocator(), m_avalon.getRenderer(),
        m_avalon.getUi());
    m_updater->setTransforms(&m_tree);
    m_updater->setRenderFrame("map");
    m_baseDrawables = renderer().drawableCount();
    m_basePrePasses = renderer().prePassCount();
  }
  void TearDown() override {
    m_updater.reset();
    m_avalon.cleanUp();
  }

  [[nodiscard]] avalon::Renderer& renderer() const {
    return *m_avalon.getRenderer();
  }
  void frames(int count) {
    for (int i = 0; i < count; ++i) {
      EXPECT_TRUE(m_avalon.frame());
    }
  }
};

TEST_F(SceneUpdaterTest, RejectsNullArguments) {
  EXPECT_THROW(SceneUpdater(nullptr, m_avalon.getAllocator(),
                            m_avalon.getRenderer(), m_avalon.getUi()),
               std::runtime_error);
  EXPECT_THROW(SceneUpdater(m_avalon.getDevice(), m_avalon.getAllocator(),
                            nullptr, m_avalon.getUi()),
               std::runtime_error);
}

TEST_F(SceneUpdaterTest, HelpersFollowTheProto3Defaults) {
  EXPECT_EQ(SceneUpdater::cameraNamespace(kImageTopic), "/CAM_FRONT");
  EXPECT_EQ(SceneUpdater::cameraNamespace(kAnnotationTopic), "/CAM_FRONT");
  EXPECT_EQ(SceneUpdater::cameraNamespace("/image"), "/image");
  EXPECT_EQ(SceneUpdater::cameraNamespace("image"), "image");
  EXPECT_EQ(SceneUpdater::toColor(Color{}), glm::vec4(1.0F));
  EXPECT_EQ(SceneUpdater::toColor(rgba(0.0F, 0.5F, 0.0F, 0.25F)),
            glm::vec4(0.0F, 0.5F, 0.0F, 0.25F));
  const glm::mat4 pose = SceneUpdater::toMatrix(poseAt(1.0, 2.0, 3.0));
  EXPECT_EQ(pose[3], glm::vec4(1.0F, 2.0F, 3.0F, 1.0F));
  EXPECT_EQ(pose[0], glm::vec4(1.0F, 0.0F, 0.0F, 0.0F));  // zero quat
}

TEST_F(SceneUpdaterTest, CubeEntitiesAreCreatedUpdatedAndDeletedThroughAPool) {
  m_updater->onSceneUpdate(kMarkers, updateWith(cubeEntity("a", kSecond, 1.0)));
  ASSERT_EQ(m_updater->entityCount(), 1U);
  const SceneUpdater::Entity* a = m_updater->entity(kMarkers, "a");
  ASSERT_NE(a, nullptr);
  ASSERT_EQ(a->boxes.size(), 1U);
  EXPECT_EQ(a->textCount, 1U);
  EXPECT_EQ(m_updater->textCount(), 1U);
  EXPECT_EQ(a->expiresAt, 0U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 1);
  // map <- base_link (1, 2, 0), cube pose (1, 0, 0), size 2.
  const glm::mat4& model = a->boxes.front()->getTransform();
  EXPECT_EQ(model[3], glm::vec4(2.0F, 2.0F, 0.0F, 1.0F));
  EXPECT_FLOAT_EQ(model[0].x, 2.0F);
  EXPECT_EQ(a->boxes.front()->getColor(), glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
  EXPECT_TRUE(a->boxes.front()->isVisible());
  frames(2);

  // An update with the same id replaces the primitives, reusing the box.
  m_updater->onSceneUpdate(kMarkers, updateWith(cubeEntity("a", kSecond, 5.0)));
  EXPECT_EQ(m_updater->entityCount(), 1U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 1);
  EXPECT_EQ(m_updater->entity(kMarkers, "a")->boxes.front()->getTransform()[3],
            glm::vec4(6.0F, 2.0F, 0.0F, 1.0F));

  // Deleting parks the box; the next entity takes it back.
  SceneUpdate deletion;
  deletion.deletions.push_back(
      {.timestamp = kSecond, .type = DeletionType::kMatchingId, .id = "a"});
  m_updater->onSceneUpdate(kMarkers, deletion);
  EXPECT_EQ(m_updater->entityCount(), 0U);
  EXPECT_EQ(m_updater->pooledCount(), 1U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 1);
  m_updater->onSceneUpdate(kMarkers, updateWith(cubeEntity("b", kSecond, 1.0)));
  m_updater->onSceneUpdate("/other", updateWith(cubeEntity("b", kSecond, 1.0)));
  EXPECT_EQ(m_updater->entityCount(), 2U);
  EXPECT_EQ(m_updater->pooledCount(), 0U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 2);

  // Delete-all only touches the topic it is published on.
  SceneUpdate all;
  all.deletions.push_back(
      {.timestamp = kSecond, .type = DeletionType::kAll, .id = ""});
  m_updater->onSceneUpdate(kMarkers, all);
  EXPECT_EQ(m_updater->entityCount(), 1U);
  EXPECT_NE(m_updater->entity("/other", "b"), nullptr);
  frames(1);

  m_updater->clear();
  EXPECT_EQ(m_updater->entityCount(), 0U);
  EXPECT_EQ(m_updater->pooledCount(), 0U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables);
  m_updater->clear();  // idempotent
}

TEST_F(SceneUpdaterTest, LifetimesExpireAgainstTheTickTime) {
  m_updater->onSceneUpdate(
      kMarkers, updateWith(cubeEntity("a", 100 * kSecond, 1.0, 50 * kSecond)));
  m_updater->onSceneUpdate(
      kMarkers, updateWith(cubeEntity("forever", 100 * kSecond, 1.0)));
  EXPECT_EQ(m_updater->entity(kMarkers, "a")->expiresAt, 150 * kSecond);
  m_updater->onTick(149 * kSecond);
  EXPECT_EQ(m_updater->entityCount(), 2U);
  m_updater->onTick(150 * kSecond);
  EXPECT_EQ(m_updater->entityCount(), 1U);
  EXPECT_EQ(m_updater->entity(kMarkers, "a"), nullptr);
  EXPECT_NE(m_updater->entity(kMarkers, "forever"), nullptr);
  // A seek drops everything.
  m_updater->onSeek(0);
  EXPECT_EQ(m_updater->entityCount(), 0U);
  EXPECT_EQ(m_updater->pooledCount(), 2U);
}

TEST_F(SceneUpdaterTest, EntityVisibilityFollowsTheTopic) {
  m_updater->onSceneUpdate(kMarkers, updateWith(cubeEntity("a", kSecond, 1.0)));
  m_updater->onTopicVisibility(kMarkers, false);
  EXPECT_FALSE(m_updater->isTopicVisible(kMarkers));
  EXPECT_FALSE(m_updater->entity(kMarkers, "a")->boxes.front()->isVisible());
  // Entities created while hidden start hidden.
  m_updater->onSceneUpdate(kMarkers, updateWith(cubeEntity("b", kSecond, 2.0)));
  EXPECT_FALSE(m_updater->entity(kMarkers, "b")->boxes.front()->isVisible());
  m_updater->onTopicVisibility(kMarkers, true);
  EXPECT_TRUE(m_updater->entity(kMarkers, "a")->boxes.front()->isVisible());
  EXPECT_TRUE(m_updater->entity(kMarkers, "b")->boxes.front()->isVisible());
}

TEST_F(SceneUpdaterTest, LinesSpheresAndArrowsBecomeDrawables) {
  SceneEntity entity;
  entity.timestamp = kSecond;
  entity.frameId = "map";
  entity.id = "shapes";
  entity.lines.push_back(line(LineType::kLineStrip, 3));  // 2 segments
  entity.lines.push_back(line(LineType::kLineLoop, 3));   // 3 segments
  entity.lines.push_back(line(LineType::kLineList, 5));   // odd point dropped
  LinePrimitive indexed = line(LineType::kLineStrip, 4);
  indexed.indices = {0, 3, 9};  // 9 is out of range
  indexed.scaleInvariant = true;
  indexed.thickness = 3.0;
  indexed.colors = {rgba(1, 0, 0, 1), rgba(0, 1, 0, 1), rgba(0, 0, 1, 1),
                    rgba(1, 1, 1, 1)};
  entity.lines.push_back(indexed);
  entity.spheres.push_back(SpherePrimitive{
      .pose = poseAt(0.0, 0.0, 1.0), .size = vec3(1.0, 2.0, 3.0), .color = {}});
  entity.arrows.push_back(ArrowPrimitive{.pose = poseAt(0.0, 0.0, 0.0),
                                         .shaftLength = 1.0,
                                         .shaftDiameter = 0.1,
                                         .headLength = 0.3,
                                         .headDiameter = 0.3,
                                         .color = rgba(0, 0, 1, 1)});
  entity.cylinderCount = 2;
  m_updater->onSceneUpdate(kMarkers, updateWith(entity));

  const SceneUpdater::Entity* shapes = m_updater->entity(kMarkers, "shapes");
  ASSERT_NE(shapes, nullptr);
  ASSERT_EQ(shapes->lines.size(), 4U);
  EXPECT_EQ(shapes->lines[0]->vertexCount(), 4U);
  EXPECT_EQ(shapes->lines[1]->vertexCount(), 6U);
  EXPECT_EQ(shapes->lines[2]->vertexCount(), 4U);
  EXPECT_EQ(shapes->lines[3]->vertexCount(), 2U);
  EXPECT_EQ(shapes->lines[0]->getLineWidth(), 1.0F);
  EXPECT_EQ(shapes->lines[3]->getLineWidth(), 3.0F);  // mock: wide lines
  ASSERT_EQ(shapes->spheres.size(), 1U);
  EXPECT_EQ(shapes->spheres.front()->getColor(), glm::vec4(1.0F));
  EXPECT_FLOAT_EQ(shapes->spheres.front()->getTransform()[1].y, 2.0F);
  ASSERT_EQ(shapes->arrows.size(), 1U);
  EXPECT_GT(shapes->arrows.front()->indexCount(), 0U);
  EXPECT_EQ(shapes->unsupportedCount, 2U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 6);
  frames(2);

  // Arrows are unique meshes and leave the renderer with the entity.
  m_updater->onSeek(0);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 5);
  EXPECT_EQ(m_updater->pooledCount(), 5U);
}

TEST_F(SceneUpdaterTest, PointCloudsAreTransformedIntoTheRenderFrame) {
  const std::vector<glm::vec4> points = {{1.0F, 0.0F, 0.0F, 0.0F},
                                         {0.0F, 1.0F, 0.0F, 50.0F},
                                         {0.0F, 0.0F, 1.0F, 100.0F}};
  m_updater->onPointCloud("/LIDAR_TOP", cloudOf("LIDAR_TOP", points));
  const SceneUpdater::Cloud* lidar = m_updater->cloud("/LIDAR_TOP");
  ASSERT_NE(lidar, nullptr);
  EXPECT_EQ(lidar->drawable->pointCount(), 3U);
  EXPECT_EQ(lidar->drawable->getPointSize(), SceneUpdater::kLidarPointSize);
  // map <- base_link (1, 2, 0) <- LIDAR_TOP (0.9, 0, 1.8).
  const glm::vec4 origin = lidar->drawable->getTransform()[3];
  EXPECT_NEAR(origin.x, 1.9F, 1e-5F);
  EXPECT_NEAR(origin.y, 2.0F, 1e-5F);
  EXPECT_NEAR(origin.z, 1.8F, 1e-5F);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 1);

  // The next message reuses the drawable; radar points are larger; a frame
  // the tree does not know is drawn in place.
  m_updater->onPointCloud("/LIDAR_TOP", cloudOf("LIDAR_TOP", points));
  EXPECT_EQ(lidar->messages, 2U);
  m_updater->onPointCloud("/RADAR_FRONT", cloudOf("nowhere", points));
  const SceneUpdater::Cloud* radar = m_updater->cloud("/RADAR_FRONT");
  ASSERT_NE(radar, nullptr);
  EXPECT_EQ(radar->drawable->getPointSize(), SceneUpdater::kRadarPointSize);
  EXPECT_EQ(radar->drawable->getTransform(), glm::mat4(1.0F));
  EXPECT_EQ(m_updater->cloudCount(), 2U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables + 2);
  frames(2);

  m_updater->onTopicVisibility("/LIDAR_TOP", false);
  EXPECT_FALSE(lidar->drawable->isVisible());
  EXPECT_TRUE(radar->drawable->isVisible());
  m_updater->onSeek(0);
  EXPECT_EQ(lidar->drawable->pointCount(), 0U);
  m_updater->clear();
  EXPECT_EQ(m_updater->cloudCount(), 0U);
  EXPECT_EQ(renderer().drawableCount(), m_baseDrawables);
}

TEST_F(SceneUpdaterTest, AnEmptyRenderFrameAdoptsTheFirstFrameSeen) {
  m_updater->setRenderFrame("");
  m_updater->onSceneUpdate(kMarkers, updateWith(cubeEntity("a", kSecond, 1.0)));
  EXPECT_EQ(m_updater->renderFrame(), "base_link");
  EXPECT_EQ(m_updater->entity(kMarkers, "a")->boxes.front()->getTransform()[3],
            glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
}

TEST_F(SceneUpdaterTest, CamerasGetATextureAndAnnotationOverlays) {
  // Two images in one tick: only the newest is decoded.
  m_updater->onImage(kImageTopic, jpegImage(kWidth, kHeight, 1));
  m_updater->onImage(kImageTopic, jpegImage(kWidth, kHeight, 2));
  EXPECT_EQ(m_updater->cameraTexture(kImageTopic), nullptr);
  m_updater->onTick(2);
  const SceneUpdater::Camera* camera = m_updater->camera(kImageTopic);
  ASSERT_NE(camera, nullptr);
  ASSERT_NE(camera->texture, nullptr);
  EXPECT_EQ(camera->texture->width(), kWidth);
  EXPECT_EQ(camera->texture->height(), kHeight);
  EXPECT_EQ(camera->texture->uploadCount(), 1U);
  EXPECT_EQ(camera->framesDecoded, 1U);
  EXPECT_EQ(camera->framesSkipped, 1U);
  EXPECT_EQ(m_updater->cameraCount(), 1U);
  EXPECT_EQ(renderer().prePassCount(), m_basePrePasses + 1);
  EXPECT_EQ(m_updater->cameraTexture(kAnnotationTopic), camera->texture);

  m_updater->onImageAnnotations(kAnnotationTopic, boxAndCircle(4));
  EXPECT_EQ(camera->texture->overlayCount(), 2U);
  ASSERT_EQ(camera->lineOverlays.size(), 2U);
  EXPECT_EQ(camera->lineOverlays[0]->vertexCount(), 8U);  // closed loop
  EXPECT_EQ(camera->lineOverlays[0]->getLineWidth(), 2.0F);
  EXPECT_EQ(camera->lineOverlays[1]->vertexCount(),
            2U * SceneUpdater::kCircleSegments);
  EXPECT_EQ(camera->textCount, 1U);
  EXPECT_EQ(m_updater->textCount(), 1U);
  frames(2);

  // Newer annotations replace the older ones; unused overlays are hidden.
  ImageAnnotations pointsOnly;
  PointsAnnotation dots;
  dots.type = PointsType::kPoints;
  dots.points = {{.x = 1.0, .y = 1.0}, {.x = 2.0, .y = 2.0}};
  pointsOnly.points.push_back(dots);
  m_updater->onImageAnnotations(kAnnotationTopic, pointsOnly);
  EXPECT_EQ(camera->linesUsed, 0U);
  EXPECT_EQ(camera->pointsUsed, 1U);
  EXPECT_FALSE(camera->lineOverlays[0]->isVisible());
  EXPECT_EQ(camera->pointOverlays[0]->pointCount(), 2U);
  EXPECT_TRUE(camera->pointOverlays[0]->isVisible());
  EXPECT_EQ(camera->texture->overlayCount(), 3U);
  EXPECT_EQ(camera->textCount, 0U);

  // Hiding the annotation topic hides the overlays in use.
  m_updater->onTopicVisibility(kAnnotationTopic, false);
  EXPECT_FALSE(camera->pointOverlays[0]->isVisible());
  m_updater->onTopicVisibility(kAnnotationTopic, true);
  EXPECT_TRUE(camera->pointOverlays[0]->isVisible());

  // A different image size recreates the texture with the overlays.
  m_updater->onImage(kImageTopic, jpegImage(32, 18, 3));
  m_updater->onTick(3);
  ASSERT_NE(camera->texture, nullptr);
  EXPECT_EQ(camera->texture->width(), 32U);
  EXPECT_EQ(camera->texture->overlayCount(), 3U);
  EXPECT_EQ(renderer().prePassCount(), m_basePrePasses + 1);
  EXPECT_EQ(camera->framesDecoded, 2U);
  frames(2);

  // A frame that is not a JPEG is a warning, not an exception.
  m_updater->onImage(kImageTopic,
                     CompressedImage{.timestamp = 4,
                                     .frameId = "CAM_FRONT",
                                     .data = std::vector<std::byte>(16),
                                     .format = "png"});
  EXPECT_NO_THROW(m_updater->onTick(4));
  EXPECT_EQ(camera->framesDecoded, 2U);

  // A seek drops the annotations and any pending frame.
  m_updater->onImage(kImageTopic, jpegImage(32, 18, 5));
  m_updater->onSeek(0);
  EXPECT_FALSE(camera->pending.has_value());
  EXPECT_EQ(camera->pointsUsed, 0U);
  m_updater->onTick(0);
  EXPECT_EQ(camera->framesDecoded, 2U);

  m_updater->clear();
  EXPECT_EQ(m_updater->cameraCount(), 0U);
  EXPECT_EQ(renderer().prePassCount(), m_basePrePasses);
  frames(1);
}

TEST_F(SceneUpdaterTest, AnnotationsBeforeTheFirstImageWaitForTheTexture) {
  m_updater->onImageAnnotations(kAnnotationTopic, boxAndCircle(3));
  const SceneUpdater::Camera* camera = m_updater->camera(kAnnotationTopic);
  ASSERT_NE(camera, nullptr);
  EXPECT_EQ(camera->texture, nullptr);
  EXPECT_EQ(camera->linesUsed, 2U);
  m_updater->onImage(kImageTopic, jpegImage(kWidth, kHeight, 1));
  m_updater->onTick(1);
  ASSERT_NE(camera->texture, nullptr);
  EXPECT_EQ(camera->texture->overlayCount(), 2U);
  frames(1);
}

TEST_F(SceneUpdaterTest, CountsCalibrationsAndUnsupportedTopics) {
  m_updater->onCameraCalibration("/CAM_FRONT/camera_info", CameraCalibration{});
  EXPECT_EQ(m_updater->calibrationCount(), 1U);
  m_updater->onUnsupported("/gps", "foxglove.LocationFix");
  m_updater->onUnsupported("/gps", "foxglove.LocationFix");
  ASSERT_EQ(m_updater->unsupportedTopics().size(), 1U);
  EXPECT_EQ(m_updater->unsupportedTopics().front(), "/gps");
}

}  // namespace camelot

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
#include <filesystem>  // NOLINT(build/c++17)
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Camelot/src/data/FoxgloveDecoder.h"
#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/JpegDecoder.h"
#include "Camelot/src/data/McapSource.h"

namespace camelot {

namespace {

// The layout of Tests/fixtures/nuscenes-mini.mcap (see make_fixture.py).
constexpr Time kSecond = 1'000'000'000ULL;
constexpr Time kMilli = 1'000'000ULL;
constexpr Time kT0 = 1'600'000'000ULL * kSecond;
constexpr Time kEnd = kT0 + 2 * kSecond;  // the last /gps message
constexpr uint64_t kCameras = 2;
constexpr uint64_t kCameraTopics = 3;  // image, annotations, camera_info
constexpr uint64_t kCamFrames = 8;
constexpr uint64_t kLidarMessages = 10;
constexpr size_t kLidarPoints = 300;
constexpr uint64_t kRadarMessages = 10;
constexpr uint64_t kMarkerMessages = 4;
constexpr uint64_t kSemanticMapMessages = 1;
constexpr uint64_t kTfMessages = 20 + 4;
constexpr uint64_t kGpsMessages = 3;
constexpr uint64_t kImuMessages = 5;
constexpr uint64_t kTotalMessages = kCameras * kCameraTopics * kCamFrames +
                                    kLidarMessages + kRadarMessages +
                                    kMarkerMessages + kSemanticMapMessages +
                                    kTfMessages + kGpsMessages + kImuMessages;
constexpr size_t kTopics = 13;
constexpr uint32_t kImageWidth = 64;
constexpr uint32_t kImageHeight = 36;

std::filesystem::path fixtures() { return {CAMELOT_TEST_FIXTURES_DIR}; }

std::filesystem::path fixture() { return fixtures() / "nuscenes-mini.mcap"; }

// Fails the test when nothing was found and returns an empty payload, so
// the decoders that follow never touch an empty optional.
std::vector<std::byte> require(std::optional<std::vector<std::byte>> found) {
  EXPECT_TRUE(found.has_value());
  return std::move(found).value_or(std::vector<std::byte>{});
}

// A message copied out of a forEach() callback.
struct Copied {
  std::string topic;
  std::string schemaName;
  Time logTime{0};
  std::vector<std::byte> data;
};

class McapSourceTest : public ::testing::Test {
 protected:
  McapSource m_source;

  void SetUp() override { m_source.open(fixture()); }

  [[nodiscard]] const TopicInfo* topic(const std::string& name) const {
    const auto& topics = m_source.topics();
    const auto found = std::ranges::find_if(
        topics, [&](const TopicInfo& t) { return t.topic == name; });
    return found == topics.end() ? nullptr : &*found;
  }

  // Every message of `topics` in [t0, t1).
  std::vector<Copied> collect(Time t0, Time t1,
                              const std::vector<std::string>& topics = {}) {
    std::vector<Copied> out;
    m_source.forEach(t0, t1, topics, [&](const RawMessage& m) {
      out.push_back({.topic = std::string(m.topic),
                     .schemaName = std::string(m.schemaName),
                     .logTime = m.logTime,
                     .data = {m.data.begin(), m.data.end()}});
    });
    return out;
  }
};

}  // namespace

TEST(McapSourceOpenTest, BadPathThrowsWithTheMcapStatus) {
  McapSource source;
  EXPECT_FALSE(source.isOpen());
  try {
    source.open(fixtures() / "does-not-exist.mcap");
    FAIL() << "expected an exception";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("mcap"), std::string::npos);
  }
  EXPECT_FALSE(source.isOpen());
  // Not an MCAP file at all.
  EXPECT_THROW(source.open(fixtures() / "make_fixture.py"), std::runtime_error);
  EXPECT_FALSE(source.isOpen());
  EXPECT_THROW(source.forEach(0, 1, {}, [](const RawMessage&) {}),
               std::runtime_error);
  EXPECT_THROW((void)source.latestBefore("/tf", 0), std::runtime_error);
}

TEST_F(McapSourceTest, ListsTopicsWithSchemaAndCounts) {
  EXPECT_TRUE(m_source.isOpen());
  EXPECT_EQ(m_source.path(), fixture());
  EXPECT_EQ(m_source.topics().size(), kTopics);

  const TopicInfo* lidar = topic("/LIDAR_TOP");
  ASSERT_NE(lidar, nullptr);
  EXPECT_EQ(lidar->schemaName, "foxglove.PointCloud");
  EXPECT_EQ(lidar->encoding, "protobuf");
  EXPECT_EQ(lidar->messageCount, kLidarMessages);

  const TopicInfo* image = topic("/CAM_FRONT/image_rect_compressed");
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(image->schemaName, "foxglove.CompressedImage");
  EXPECT_EQ(image->messageCount, kCamFrames);
  const TopicInfo* annotations = topic("/CAM_BACK/annotations");
  ASSERT_NE(annotations, nullptr);
  EXPECT_EQ(annotations->schemaName, "foxglove.ImageAnnotations");
  const TopicInfo* info = topic("/CAM_BACK/camera_info");
  ASSERT_NE(info, nullptr);
  EXPECT_EQ(info->schemaName, "foxglove.CameraCalibration");
  const TopicInfo* radar = topic("/RADAR_FRONT");
  ASSERT_NE(radar, nullptr);
  EXPECT_EQ(radar->schemaName, "foxglove.PointCloud");
  const TopicInfo* markers = topic("/markers/annotations");
  ASSERT_NE(markers, nullptr);
  EXPECT_EQ(markers->schemaName, "foxglove.SceneUpdate");
  const TopicInfo* lanes = topic("/semantic_map");
  ASSERT_NE(lanes, nullptr);
  EXPECT_EQ(lanes->messageCount, kSemanticMapMessages);
  const TopicInfo* tf = topic("/tf");
  ASSERT_NE(tf, nullptr);
  EXPECT_EQ(tf->schemaName, "foxglove.FrameTransform");
  EXPECT_EQ(tf->messageCount, kTfMessages);
  const TopicInfo* gps = topic("/gps");
  ASSERT_NE(gps, nullptr);
  EXPECT_EQ(gps->schemaName, "foxglove.LocationFix");
  const TopicInfo* imu = topic("/imu");
  ASSERT_NE(imu, nullptr);
  EXPECT_EQ(imu->encoding, "json");
  EXPECT_EQ(imu->messageCount, kImuMessages);
}

TEST_F(McapSourceTest, ReportsTheTimeRange) {
  EXPECT_EQ(m_source.startTime(), kT0);
  EXPECT_EQ(m_source.endTime(), kEnd);
  EXPECT_EQ(m_source.messageCount(), kTotalMessages);
}

TEST_F(McapSourceTest, IteratesEverythingInLogTimeOrder) {
  const auto all = collect(0, kEnd + 1);
  EXPECT_EQ(all.size(), kTotalMessages);
  for (size_t i = 1; i < all.size(); ++i) {
    EXPECT_LE(all[i - 1].logTime, all[i].logTime) << "message " << i;
  }
  EXPECT_EQ(all.front().logTime, kT0);
  EXPECT_EQ(all.back().logTime, kEnd);
  EXPECT_EQ(all.back().topic, "/gps");
  EXPECT_EQ(all.back().schemaName, "foxglove.LocationFix");
}

TEST_F(McapSourceTest, TimeWindowIsHalfOpen) {
  // Lidar messages every 200 ms: 600 and 800 ms fall in [500, 1000).
  const auto window =
      collect(kT0 + 500 * kMilli, kT0 + kSecond, {"/LIDAR_TOP"});
  ASSERT_EQ(window.size(), 2U);
  EXPECT_EQ(window[0].logTime, kT0 + 600 * kMilli);
  EXPECT_EQ(window[1].logTime, kT0 + 800 * kMilli);
  // The end is exclusive, the start inclusive.
  EXPECT_EQ(
      collect(kT0 + 600 * kMilli, kT0 + 800 * kMilli, {"/LIDAR_TOP"}).size(),
      1U);
  EXPECT_TRUE(
      collect(kT0 + 601 * kMilli, kT0 + 800 * kMilli, {"/LIDAR_TOP"}).empty());
  EXPECT_TRUE(collect(kT0, kT0, {"/LIDAR_TOP"}).empty());
  EXPECT_TRUE(collect(kEnd + 1, kEnd + kSecond).empty());
}

TEST_F(McapSourceTest, FiltersByTopic) {
  const auto cameras = collect(
      0, kEnd + 1,
      {"/CAM_FRONT/image_rect_compressed", "/CAM_BACK/image_rect_compressed"});
  EXPECT_EQ(cameras.size(), kCameras * kCamFrames);
  for (const auto& m : cameras) {
    EXPECT_EQ(m.schemaName, "foxglove.CompressedImage");
  }
  EXPECT_TRUE(collect(0, kEnd + 1, {"/nope"}).empty());
}

TEST_F(McapSourceTest, LatestBeforeFindsTheLastMessageAtOrBeforeT) {
  // Lidar at 0, 200, 400, ... ms.
  const PointCloud at700 = decodePointCloud(
      require(m_source.latestBefore("/LIDAR_TOP", kT0 + 700 * kMilli)));
  EXPECT_EQ(at700.timestamp, kT0 + 600 * kMilli);

  const PointCloud exact = decodePointCloud(
      require(m_source.latestBefore("/LIDAR_TOP", kT0 + 800 * kMilli)));
  EXPECT_EQ(exact.timestamp, kT0 + 800 * kMilli);

  const PointCloud first =
      decodePointCloud(require(m_source.latestBefore("/LIDAR_TOP", kT0)));
  EXPECT_EQ(first.timestamp, kT0);

  EXPECT_FALSE(m_source.latestBefore("/LIDAR_TOP", kT0 - 1).has_value());
  EXPECT_FALSE(m_source.latestBefore("/unknown", kEnd).has_value());

  // Far past the last message: the last one.
  const PointCloud last = decodePointCloud(
      require(m_source.latestBefore("/LIDAR_TOP", kEnd + 10 * kSecond)));
  EXPECT_EQ(last.timestamp, kT0 + (kLidarMessages - 1) * 200 * kMilli);

  // A topic with a single early message is found from the end of the file.
  const SceneUpdate lanes =
      decodeSceneUpdate(require(m_source.latestBefore("/semantic_map", kEnd)));
  EXPECT_EQ(lanes.entities.size(), 1U);

  // The last /tf message is the dynamic one at 1.9 s.
  const FrameTransform transform =
      decodeFrameTransform(require(m_source.latestBefore("/tf", kEnd)));
  EXPECT_EQ(transform.timestamp, kT0 + 1900 * kMilli);
  EXPECT_EQ(transform.parentFrameId, "map");
  EXPECT_EQ(transform.childFrameId, "base_link");
  EXPECT_DOUBLE_EQ(transform.translation.x, 9.5);
}

TEST_F(McapSourceTest, LatestBeforeAgreesWithIteration) {
  // Every camera frame found through the index equals the one iteration
  // yields, across the chunk boundaries of the small-chunk fixture.
  const std::string topic = "/CAM_BACK/image_rect_compressed";
  const auto frames = collect(0, kEnd + 1, {topic});
  ASSERT_EQ(frames.size(), kCamFrames);
  for (const auto& frame : frames) {
    EXPECT_EQ(require(m_source.latestBefore(topic, frame.logTime + 1)),
              frame.data);
  }
}

TEST_F(McapSourceTest, DecodesPointCloudsAndImages) {
  const PointCloud cloud =
      decodePointCloud(require(m_source.latestBefore("/LIDAR_TOP", kT0)));
  EXPECT_EQ(cloud.frameId, "LIDAR_TOP");
  EXPECT_EQ(cloud.pointStride, 16U);
  ASSERT_EQ(cloud.fields.size(), 4U);
  EXPECT_EQ(cloud.fields[3].name, "intensity");
  EXPECT_EQ(cloud.pointCount(), kLidarPoints);
  const auto points = cloud.positionsAndIntensity();
  ASSERT_EQ(points.size(), kLidarPoints);
  EXPECT_FLOAT_EQ(points[0].w, 0.0F);
  EXPECT_FLOAT_EQ(points[1].w, 1.0F);
  EXPECT_NEAR(points[0].x, 5.0F, 1e-4F);  // radius 5 at angle 0
  EXPECT_NEAR(points[0].z, -1.0F, 1e-4F);

  const PointCloud radar =
      decodePointCloud(require(m_source.latestBefore("/RADAR_FRONT", kEnd)));
  EXPECT_EQ(radar.frameId, "RADAR_FRONT");
  EXPECT_EQ(radar.pointCount(), 24U);

  const CompressedImage compressed = decodeCompressedImage(
      require(m_source.latestBefore("/CAM_FRONT/image_rect_compressed", kT0)));
  EXPECT_EQ(compressed.format, "jpeg");
  EXPECT_EQ(compressed.frameId, "CAM_FRONT");
  EXPECT_EQ(compressed.timestamp, kT0);
  ASSERT_TRUE(JpegDecoder::looksLikeJpeg(compressed.data));
  const DecodedImage decoded = JpegDecoder::decodeJpeg(compressed.data);
  EXPECT_EQ(decoded.width, kImageWidth);
  EXPECT_EQ(decoded.height, kImageHeight);
  EXPECT_EQ(decoded.rgba.size(), kImageWidth * kImageHeight * 4U);

  const ImageAnnotations a = decodeImageAnnotations(
      require(m_source.latestBefore("/CAM_FRONT/annotations", kT0)));
  EXPECT_EQ(a.timestamp, kT0);
  ASSERT_EQ(a.points.size(), 1U);
  EXPECT_EQ(a.points[0].type, PointsType::kLineLoop);
  EXPECT_EQ(a.points[0].points.size(), 4U);
  EXPECT_DOUBLE_EQ(a.points[0].thickness, 2.0);
  EXPECT_FLOAT_EQ(a.points[0].outlineColor.g, 1.0F);
  ASSERT_EQ(a.texts.size(), 1U);
  EXPECT_EQ(a.texts[0].text, "car");

  const CameraCalibration c = decodeCameraCalibration(
      require(m_source.latestBefore("/CAM_FRONT/camera_info", kT0)));
  EXPECT_EQ(c.width, kImageWidth);
  EXPECT_EQ(c.height, kImageHeight);
  EXPECT_EQ(c.frameId, "CAM_FRONT");
  EXPECT_EQ(c.distortionModel, "plumb_bob");
  EXPECT_EQ(c.D.size(), 5U);
  EXPECT_DOUBLE_EQ(c.K[0], 50.0);
  EXPECT_DOUBLE_EQ(c.K[2], 32.0);
  EXPECT_DOUBLE_EQ(c.P[5], 50.0);
}

TEST_F(McapSourceTest, DecodesSceneUpdatesAndTransforms) {
  const SceneUpdate objects = decodeSceneUpdate(
      require(m_source.latestBefore("/markers/annotations", kT0)));
  EXPECT_TRUE(objects.deletions.empty());
  ASSERT_EQ(objects.entities.size(), 3U);
  const SceneEntity& object = objects.entities[1];
  EXPECT_EQ(object.id, "obj-1");
  EXPECT_EQ(object.frameId, "map");
  EXPECT_EQ(object.lifetime, 500 * kMilli);
  ASSERT_EQ(object.cubes.size(), 1U);
  EXPECT_DOUBLE_EQ(object.cubes[0].size.x, 4.0);
  EXPECT_DOUBLE_EQ(object.cubes[0].pose.orientation.w, 1.0);
  ASSERT_EQ(object.texts.size(), 1U);
  EXPECT_EQ(object.texts[0].text, "vehicle.car 1");

  const SceneUpdate map =
      decodeSceneUpdate(require(m_source.latestBefore("/semantic_map", kT0)));
  ASSERT_EQ(map.entities.size(), 1U);
  EXPECT_EQ(map.entities[0].id, "lane-0");
  ASSERT_EQ(map.entities[0].lines.size(), 3U);
  const LinePrimitive& lane = map.entities[0].lines[2];
  EXPECT_EQ(lane.type, LineType::kLineStrip);
  EXPECT_EQ(lane.points.size(), 10U);
  EXPECT_DOUBLE_EQ(lane.thickness, 0.2);
  EXPECT_FLOAT_EQ(lane.color.g, 0.8F);

  // The static transforms share T0 with the first dynamic one, so iterate
  // instead of asking for the latest.
  size_t staticCount = 0;
  m_source.forEach(kT0, kT0 + 1, std::vector<std::string>{"/tf"},
                   [&](const RawMessage& m) {
                     const FrameTransform tf = decodeFrameTransform(m.data);
                     if (tf.parentFrameId == "base_link") {
                       ++staticCount;
                       if (tf.childFrameId == "LIDAR_TOP") {
                         EXPECT_DOUBLE_EQ(tf.translation.z, 1.8);
                       }
                     }
                   });
  EXPECT_EQ(staticCount, 4U);
}

TEST_F(McapSourceTest, CloseAndReopen) {
  m_source.close();
  EXPECT_FALSE(m_source.isOpen());
  EXPECT_TRUE(m_source.topics().empty());
  m_source.open(fixture());
  EXPECT_TRUE(m_source.isOpen());
  EXPECT_EQ(m_source.topics().size(), kTopics);
  const McapSource moved(std::move(m_source));
  EXPECT_TRUE(moved.isOpen());
  EXPECT_EQ(moved.messageCount(), kTotalMessages);
}

}  // namespace camelot

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
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/McapSource.h"
#include "Camelot/src/replay/IRecordingSink.h"
#include "Camelot/src/replay/Recording.h"

namespace camelot {

namespace {

// The layout of Tests/fixtures/nuscenes-mini.mcap (see make_fixture.py).
constexpr Time kSecond = 1'000'000'000ULL;
constexpr Time kMilli = 1'000'000ULL;
constexpr Time kT0 = 1'600'000'000ULL * kSecond;
constexpr Time kEnd = kT0 + 2 * kSecond;
constexpr size_t kTopics = 13;
constexpr size_t kSupportedTopics = 11;  // everything but /gps and /imu
constexpr size_t kStreamTopics = 10;     // supported minus /tf
constexpr size_t kTfSamples = 24;
constexpr uint64_t kStreamMessages = 2 * 3 * 8 + 10 + 10 + 4 + 1;

std::filesystem::path fixture() {
  return std::filesystem::path(CAMELOT_TEST_FIXTURES_DIR) /
         "nuscenes-mini.mcap";
}

// Counts every callback and keeps the message timestamps in arrival order.
class CountingSink : public IRecordingSink {
 public:
  std::map<std::string, uint64_t> perTopic;
  std::vector<Time> timestamps;
  std::vector<std::string> unsupported;
  std::vector<std::pair<std::string, bool>> visibility;
  uint64_t seeks{0};
  uint64_t ticks{0};
  Time lastTick{0};
  std::vector<Time> entityTimestamps;
  size_t lidarPoints{0};

  void onPointCloud(const std::string& topic, PointCloud cloud) override {
    ++perTopic[topic];
    timestamps.push_back(cloud.timestamp);
    if (topic == "/LIDAR_TOP") {
      lidarPoints = cloud.pointCount();
    }
  }
  void onSceneUpdate(const std::string& topic, SceneUpdate update) override {
    ++perTopic[topic];
    for (const SceneEntity& entity : update.entities) {
      timestamps.push_back(entity.timestamp);
      entityTimestamps.push_back(entity.timestamp);
    }
  }
  void onImage(const std::string& topic, CompressedImage image) override {
    ++perTopic[topic];
    timestamps.push_back(image.timestamp);
  }
  void onImageAnnotations(const std::string& topic,
                          ImageAnnotations annotations) override {
    ++perTopic[topic];
    timestamps.push_back(annotations.timestamp);
  }
  void onCameraCalibration(const std::string& topic,
                           CameraCalibration calibration) override {
    ++perTopic[topic];
    timestamps.push_back(calibration.timestamp);
  }
  void onUnsupported(const std::string& topic,
                     const std::string& schemaName) override {
    unsupported.push_back(topic + " " + schemaName);
  }
  void onSeek(Time t) override {
    (void)t;
    ++seeks;
  }
  void onTick(Time now) override {
    ++ticks;
    lastTick = now;
  }
  void onTopicVisibility(const std::string& topic, bool visible) override {
    visibility.emplace_back(topic, visible);
  }

  [[nodiscard]] uint64_t total() const {
    uint64_t sum = 0;
    for (const auto& [topic, count] : perTopic) {
      sum += count;
    }
    return sum;
  }
  [[nodiscard]] bool ordered() const {
    return std::ranges::is_sorted(timestamps);
  }
};

class RecordingTest : public ::testing::Test {
 protected:
  CountingSink m_sink;
  Recording m_recording;

  void SetUp() override {
    m_recording.setSink(&m_sink);
    m_recording.open(fixture());
  }
};

}  // namespace

TEST(RecordingOpenTest, BadPathThrowsAndStaysClosed) {
  Recording recording;
  EXPECT_FALSE(recording.isOpen());
  try {
    recording.open(std::filesystem::path(CAMELOT_TEST_FIXTURES_DIR) /
                   "missing.mcap");
    FAIL() << "expected an exception";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("mcap"), std::string::npos);
  }
  EXPECT_FALSE(recording.isOpen());
  EXPECT_TRUE(recording.topics().empty());
  // A closed recording ignores ticks and seeks.
  recording.tick(1.0);
  recording.seek(kT0);
  EXPECT_EQ(recording.deliveredCount(), 0U);
}

TEST(RecordingOpenTest, SupportedSchemasAreTheRenderedOnes) {
  EXPECT_TRUE(Recording::isSupportedSchema("foxglove.PointCloud"));
  EXPECT_TRUE(Recording::isSupportedSchema("foxglove.SceneUpdate"));
  EXPECT_TRUE(Recording::isSupportedSchema("foxglove.CompressedImage"));
  EXPECT_TRUE(Recording::isSupportedSchema("foxglove.ImageAnnotations"));
  EXPECT_TRUE(Recording::isSupportedSchema("foxglove.CameraCalibration"));
  EXPECT_TRUE(Recording::isSupportedSchema("foxglove.FrameTransform"));
  EXPECT_FALSE(Recording::isSupportedSchema("foxglove.LocationFix"));
  EXPECT_FALSE(Recording::isSupportedSchema("foxglove.Grid"));
  EXPECT_FALSE(Recording::isSupportedSchema(""));
}

TEST_F(RecordingTest, ClassifiesTopicsAndAnnouncesUnsupportedOnes) {
  EXPECT_TRUE(m_recording.isOpen());
  EXPECT_EQ(m_recording.path(), fixture());
  ASSERT_EQ(m_recording.topics().size(), kTopics);
  EXPECT_EQ(std::ranges::count_if(
                m_recording.topics(),
                [](const RecordingTopic& t) { return t.supported; }),
            static_cast<std::ptrdiff_t>(kSupportedTopics));
  const RecordingTopic* gps = m_recording.topic("/gps");
  ASSERT_NE(gps, nullptr);
  EXPECT_FALSE(gps->supported);
  EXPECT_EQ(gps->schemaName, "foxglove.LocationFix");
  const RecordingTopic* lidar = m_recording.topic("/LIDAR_TOP");
  ASSERT_NE(lidar, nullptr);
  EXPECT_TRUE(lidar->supported);
  EXPECT_TRUE(lidar->visible);
  EXPECT_EQ(lidar->messageCount, 10U);
  EXPECT_EQ(m_recording.topic("/nope"), nullptr);

  ASSERT_EQ(m_sink.unsupported.size(), 2U);
  EXPECT_EQ(m_sink.unsupported[0], "/gps foxglove.LocationFix");
  // The JSON channel carries a generated schema name.
  EXPECT_EQ(m_sink.unsupported[1].rfind("/imu ", 0), 0U);
  // Setting the sink again on an open recording announces them again.
  CountingSink other;
  m_recording.setSink(&other);
  EXPECT_EQ(other.unsupported.size(), 2U);
}

TEST_F(RecordingTest, PreloadsEveryTransformAndRendersInMap) {
  const TransformTree& tree = m_recording.transforms();
  EXPECT_EQ(tree.frames().size(), 6U);
  EXPECT_EQ(tree.sampleCount("base_link"), kTfSamples - 4);
  EXPECT_EQ(tree.sampleCount("LIDAR_TOP"), 1U);
  EXPECT_EQ(m_recording.renderFrame(), "map");
  // Lookups work at any time before playback delivered anything.
  EXPECT_TRUE(tree.lookup("map", "LIDAR_TOP", kEnd).has_value());
  EXPECT_EQ(m_sink.total(), 0U);
}

TEST_F(RecordingTest, SetsThePlaybackRange) {
  const Playback& playback = m_recording.playback();
  EXPECT_EQ(playback.start(), kT0);
  EXPECT_EQ(playback.end(), kEnd);
  EXPECT_EQ(playback.current(), kT0);
  EXPECT_FALSE(playback.isPlaying());
}

TEST_F(RecordingTest, TickDeliversTheMessagesOfTheAdvancedIntervalInOrder) {
  // Paused: nothing but the tick callback.
  m_recording.tick(1.0);
  EXPECT_EQ(m_sink.total(), 0U);
  EXPECT_EQ(m_sink.ticks, 1U);

  m_recording.playback().play();
  m_recording.tick(0.5);  // [T0, T0 + 500 ms)
  EXPECT_EQ(m_recording.playback().current(), kT0 + 500 * kMilli);
  EXPECT_EQ(m_sink.lastTick, kT0 + 500 * kMilli);
  EXPECT_EQ(m_sink.perTopic["/CAM_FRONT/image_rect_compressed"], 2U);
  EXPECT_EQ(m_sink.perTopic["/CAM_BACK/annotations"], 2U);
  EXPECT_EQ(m_sink.perTopic["/CAM_BACK/camera_info"], 2U);
  EXPECT_EQ(m_sink.perTopic["/LIDAR_TOP"], 3U);    // 0, 200, 400 ms
  EXPECT_EQ(m_sink.perTopic["/RADAR_FRONT"], 3U);  // 50, 250, 450 ms
  EXPECT_EQ(m_sink.perTopic["/markers/annotations"], 1U);
  EXPECT_EQ(m_sink.perTopic["/semantic_map"], 1U);
  EXPECT_EQ(m_sink.perTopic.count("/tf"), 0U);  // preloaded, not streamed
  EXPECT_EQ(m_sink.perTopic.count("/gps"), 0U);
  EXPECT_EQ(m_sink.total(), 20U);
  EXPECT_EQ(m_sink.lidarPoints, 300U);
  EXPECT_TRUE(m_sink.ordered());
  EXPECT_EQ(m_recording.deliveredCount(), 20U);
  EXPECT_EQ(m_recording.decodeErrorCount(), 0U);

  // The rest of the file, including the messages logged exactly at the end.
  m_recording.tick(5.0);
  EXPECT_EQ(m_sink.total(), kStreamMessages);
  EXPECT_TRUE(m_sink.ordered());
  EXPECT_TRUE(m_recording.playback().atEnd());
  EXPECT_FALSE(m_recording.playback().isPlaying());
  EXPECT_EQ(m_sink.seeks, 0U);
}

TEST_F(RecordingTest, SeekReloadsTheLatestMessageOfEveryTopic) {
  m_recording.seek(kT0 + 1700 * kMilli);
  EXPECT_EQ(m_sink.seeks, 1U);
  EXPECT_EQ(m_sink.ticks, 1U);
  EXPECT_EQ(m_sink.lastTick, kT0 + 1700 * kMilli);
  EXPECT_EQ(m_recording.playback().current(), kT0 + 1700 * kMilli);
  // One message per stream topic, each the latest before the seek time.
  EXPECT_EQ(m_sink.perTopic.size(), kStreamTopics);
  EXPECT_EQ(m_sink.total(), kStreamTopics);
  ASSERT_EQ(m_sink.entityTimestamps.size(), 4U);  // 3 markers + the lanes
  EXPECT_EQ(std::ranges::count(m_sink.entityTimestamps, kT0 + 1500 * kMilli),
            3);
  EXPECT_EQ(std::ranges::count(m_sink.entityTimestamps, kT0), 1);

  // Before the first radar message there is nothing to reload for it.
  m_recording.seek(kT0 + 10 * kMilli);
  EXPECT_EQ(m_sink.seeks, 2U);
  EXPECT_EQ(m_sink.perTopic["/RADAR_FRONT"], 1U);  // unchanged
  EXPECT_EQ(m_sink.perTopic["/LIDAR_TOP"], 2U);

  // A seek past the end clamps.
  m_recording.seek(kEnd + kSecond);
  EXPECT_EQ(m_recording.playback().current(), kEnd);
}

TEST_F(RecordingTest, LoopWrapsAndReloadsTheStart) {
  m_recording.playback().setLoop(true);
  m_recording.playback().play();
  m_recording.playback().seek(kEnd - 100 * kMilli);
  m_recording.tick(0.2);
  EXPECT_EQ(m_sink.seeks, 1U);
  EXPECT_EQ(m_recording.playback().current(), kT0);
  EXPECT_TRUE(m_recording.playback().isPlaying());
  // Nothing supported is logged in the last 100 ms; the reload at the start
  // delivers the messages logged at T0 (no radar yet).
  EXPECT_EQ(m_sink.perTopic["/semantic_map"], 1U);
  EXPECT_EQ(m_sink.perTopic["/LIDAR_TOP"], 1U);
  EXPECT_EQ(m_sink.perTopic["/markers/annotations"], 1U);
  EXPECT_EQ(m_sink.perTopic.count("/RADAR_FRONT"), 0U);
  EXPECT_EQ(m_sink.total(), 9U);
  EXPECT_EQ(m_sink.lastTick, kT0);
}

TEST_F(RecordingTest, TopicVisibilityIsForwardedToTheSink) {
  m_recording.setTopicVisible("/LIDAR_TOP", false);
  m_recording.setTopicVisible("/nope", false);
  ASSERT_EQ(m_sink.visibility.size(), 1U);
  EXPECT_EQ(m_sink.visibility[0].first, "/LIDAR_TOP");
  EXPECT_FALSE(m_sink.visibility[0].second);
  EXPECT_FALSE(m_recording.topic("/LIDAR_TOP")->visible);
  m_recording.setTopicVisible("/LIDAR_TOP", true);
  EXPECT_TRUE(m_recording.topic("/LIDAR_TOP")->visible);
}

TEST_F(RecordingTest, CorruptMessagesAreWarningsNotExceptions) {
  // A length-delimited field that claims more bytes than there are.
  constexpr std::array<std::byte, 3> kCorrupt = {
      std::byte{0x0A}, std::byte{0x7F}, std::byte{0x00}};
  const RawMessage message{.topic = "/LIDAR_TOP",
                           .schemaName = "foxglove.PointCloud",
                           .logTime = kT0,
                           .data = kCorrupt};
  EXPECT_NO_THROW({
    EXPECT_FALSE(m_recording.apply(message));
    EXPECT_FALSE(m_recording.apply(message));  // warned once, counted twice
  });
  EXPECT_EQ(m_recording.decodeErrorCount(), 2U);
  EXPECT_EQ(m_sink.total(), 0U);
  // An unknown schema is a decode error as well.
  const RawMessage unknown{.topic = "/odd",
                           .schemaName = "foxglove.Grid",
                           .logTime = kT0,
                           .data = kCorrupt};
  EXPECT_FALSE(m_recording.apply(unknown));
  EXPECT_EQ(m_recording.decodeErrorCount(), 3U);
  // Playback continues afterwards.
  m_recording.playback().play();
  m_recording.tick(0.1);
  EXPECT_GT(m_sink.total(), 0U);
}

TEST_F(RecordingTest, CloseForgetsEverything) {
  m_recording.close();
  EXPECT_FALSE(m_recording.isOpen());
  EXPECT_TRUE(m_recording.topics().empty());
  EXPECT_TRUE(m_recording.transforms().frames().empty());
  EXPECT_TRUE(m_recording.renderFrame().empty());
  // Reopening works.
  m_recording.open(fixture());
  EXPECT_EQ(m_recording.topics().size(), kTopics);
}

}  // namespace camelot

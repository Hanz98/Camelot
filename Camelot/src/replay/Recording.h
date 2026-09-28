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

#ifndef CAMELOT_SRC_REPLAY_RECORDING_H_
#define CAMELOT_SRC_REPLAY_RECORDING_H_

#include <cstddef>
#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <functional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/McapSource.h"
#include "Camelot/src/data/Playback.h"
#include "Camelot/src/data/TransformTree.h"
#include "Camelot/src/replay/IRecordingSink.h"

namespace camelot {

// One topic of an open Recording as the UI sees it.
struct RecordingTopic {
  std::string topic;
  std::string schemaName;
  uint64_t messageCount{0};
  // The schema is one Camelot decodes and renders.
  bool supported{false};
  // UI flag, forwarded to the sink through Recording::setTopicVisible().
  bool visible{true};
};

// An MCAP recording under replay: the file, the transform tree built from
// every FrameTransform message up front (so lookups work after any seek),
// the playback clock and the per-topic state. tick() decodes the messages of
// the interval the clock advanced by and hands them to the sink; seek()
// reloads the latest message of every supported topic so the scene shows
// the state at that time. No Vulkan here: the class is fully testable with
// the fixture recording.
class Recording {
 public:
  static constexpr const char* kFrameTransformSchema =
      "foxglove.FrameTransform";
  static constexpr const char* kPointCloudSchema = "foxglove.PointCloud";
  static constexpr const char* kSceneUpdateSchema = "foxglove.SceneUpdate";
  static constexpr const char* kCompressedImageSchema =
      "foxglove.CompressedImage";
  static constexpr const char* kImageAnnotationsSchema =
      "foxglove.ImageAnnotations";
  static constexpr const char* kCameraCalibrationSchema =
      "foxglove.CameraCalibration";
  static constexpr const char* kMapFrame = "map";

  // The sink is not owned and may be null (messages are then only counted).
  // Setting a sink on an open recording announces the unsupported topics.
  void setSink(IRecordingSink* sink);
  [[nodiscard]] IRecordingSink* sink() const { return m_sink; }

  // Opens the file, preloads the transforms, classifies the topics and sets
  // the playback range. Throws std::runtime_error (with the mcap status
  // text) and leaves the recording closed on failure.
  void open(const std::filesystem::path& path);
  void close();
  [[nodiscard]] bool isOpen() const { return m_source.isOpen(); }
  [[nodiscard]] const std::filesystem::path& path() const {
    return m_source.path();
  }

  // Advances the clock and delivers the messages of the advanced interval;
  // reloads the state at the start when the playback wrapped. Ends with
  // IRecordingSink::onTick().
  void tick(double wallSeconds);
  // Moves the clock to `t` (clamped) and reloads the latest message at or
  // before it of every supported topic.
  void seek(Time t);

  [[nodiscard]] const std::vector<RecordingTopic>& topics() const {
    return m_topics;
  }
  [[nodiscard]] const RecordingTopic* topic(std::string_view name) const;
  // Updates the flag and forwards it to the sink. Unknown topics are ignored.
  void setTopicVisible(std::string_view name, bool visible);

  [[nodiscard]] Playback& playback() { return m_playback; }
  [[nodiscard]] const Playback& playback() const { return m_playback; }
  [[nodiscard]] const TransformTree& transforms() const { return m_tree; }
  // The frame the scene is expressed in: `map` when the tree has it, else
  // the first root frame of the tree, else the frame of the first entity or
  // point cloud delivered (empty until then).
  [[nodiscard]] const std::string& renderFrame() const { return m_renderFrame; }

  // Decodes one message and hands it to the sink. A decode failure is
  // logged once per topic (spdlog::warn), counted and returns false; it
  // never throws. Public so tests can feed a corrupt message.
  bool apply(const RawMessage& message);

  [[nodiscard]] uint64_t deliveredCount() const { return m_delivered; }
  [[nodiscard]] uint64_t decodeErrorCount() const { return m_decodeErrors; }

  [[nodiscard]] static bool isSupportedSchema(std::string_view schemaName);

 private:
  McapSource m_source;
  TransformTree m_tree;
  Playback m_playback;
  IRecordingSink* m_sink{nullptr};
  std::vector<RecordingTopic> m_topics;
  // Supported topics other than the transforms: what tick() and seek() read.
  std::vector<std::string> m_streamTopics;
  std::string m_renderFrame;
  std::set<std::string, std::less<>> m_warnedTopics;
  uint64_t m_delivered{0};
  uint64_t m_decodeErrors{0};

  void classifyTopics();
  void preloadTransforms();
  void chooseRenderFrame();
  void announceUnsupported();
  // Delivers the latest message at or before `t` of every stream topic.
  void reload(Time t);
  void dispatch(const std::string& topic, std::string_view schemaName,
                std::span<const std::byte> data);
  void adoptRenderFrame(const std::string& frameId);
};

}  // namespace camelot

#endif  // CAMELOT_SRC_REPLAY_RECORDING_H_

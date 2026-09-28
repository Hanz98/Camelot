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

#include "Camelot/src/replay/Recording.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Camelot/src/data/FoxgloveDecoder.h"

namespace camelot {

namespace {

constexpr std::array<std::string_view, 6> kSupportedSchemas = {
    Recording::kFrameTransformSchema,   Recording::kPointCloudSchema,
    Recording::kSceneUpdateSchema,      Recording::kCompressedImageSchema,
    Recording::kImageAnnotationsSchema, Recording::kCameraCalibrationSchema,
};

}  // namespace

bool Recording::isSupportedSchema(std::string_view schemaName) {
  return std::ranges::find(kSupportedSchemas, schemaName) !=
         kSupportedSchemas.end();
}

void Recording::setSink(IRecordingSink* sink) {
  m_sink = sink;
  if (isOpen()) {
    announceUnsupported();
  }
}

void Recording::open(const std::filesystem::path& path) {
  close();
  m_source.open(path);  // throws with the mcap status text
  classifyTopics();
  preloadTransforms();
  chooseRenderFrame();
  m_playback = Playback{};
  m_playback.setRange(m_source.startTime(), m_source.endTime());
  spdlog::info(
      "Recording: opened {} ({} topics, {} supported, {} messages, {:.2f} s)",
      path.string(), m_topics.size(),
      std::ranges::count_if(
          m_topics, [](const RecordingTopic& t) { return t.supported; }),
      m_source.messageCount(),
      static_cast<double>(m_source.endTime() - m_source.startTime()) * 1e-9);
  announceUnsupported();
}

void Recording::close() {
  m_source.close();
  m_tree.clear();
  m_playback = Playback{};
  m_topics.clear();
  m_streamTopics.clear();
  m_renderFrame.clear();
  m_warnedTopics.clear();
  m_delivered = 0;
  m_decodeErrors = 0;
}

void Recording::classifyTopics() {
  m_topics.clear();
  m_streamTopics.clear();
  for (const TopicInfo& info : m_source.topics()) {
    const bool supported = isSupportedSchema(info.schemaName);
    m_topics.push_back({.topic = info.topic,
                        .schemaName = info.schemaName,
                        .messageCount = info.messageCount,
                        .supported = supported,
                        .visible = true});
    if (supported && info.schemaName != kFrameTransformSchema) {
      m_streamTopics.push_back(info.topic);
    }
  }
}

void Recording::preloadTransforms() {
  std::vector<std::string> tfTopics;
  for (const RecordingTopic& topic : m_topics) {
    if (topic.schemaName == kFrameTransformSchema) {
      tfTopics.push_back(topic.topic);
    }
  }
  if (tfTopics.empty()) {
    return;
  }
  size_t loaded = 0;
  m_source.forEach(
      0, m_source.endTime() + 1, tfTopics, [&](const RawMessage& message) {
        try {
          m_tree.add(decodeFrameTransform(message.data));
          ++loaded;
        } catch (const std::exception& error) {
          ++m_decodeErrors;
          if (m_warnedTopics.insert(std::string(message.topic)).second) {
            spdlog::warn("Recording: {}: {}", message.topic, error.what());
          }
        }
      });
  std::string frames;
  for (const std::string& frame : m_tree.frames()) {
    frames += frames.empty() ? frame : " " + frame;
  }
  spdlog::info("Recording: {} transforms, frames: {}", loaded, frames);
}

void Recording::chooseRenderFrame() {
  m_renderFrame.clear();
  const std::vector<std::string> frames = m_tree.frames();
  if (std::ranges::find(frames, kMapFrame) != frames.end()) {
    m_renderFrame = kMapFrame;
    return;
  }
  for (const std::string& frame : frames) {
    if (!m_tree.parent(frame).has_value()) {
      m_renderFrame = frame;
      return;
    }
  }
}

void Recording::adoptRenderFrame(const std::string& frameId) {
  if (m_renderFrame.empty() && !frameId.empty()) {
    m_renderFrame = frameId;
    spdlog::info("Recording: rendering in frame '{}'", frameId);
  }
}

void Recording::announceUnsupported() {
  if (m_sink == nullptr) {
    return;
  }
  for (const RecordingTopic& topic : m_topics) {
    if (!topic.supported) {
      m_sink->onUnsupported(topic.topic, topic.schemaName);
    }
  }
}

const RecordingTopic* Recording::topic(std::string_view name) const {
  const auto found = std::ranges::find(m_topics, name, &RecordingTopic::topic);
  return found == m_topics.end() ? nullptr : &*found;
}

void Recording::setTopicVisible(std::string_view name, bool visible) {
  const auto found = std::ranges::find(m_topics, name, &RecordingTopic::topic);
  if (found == m_topics.end()) {
    return;
  }
  found->visible = visible;
  if (m_sink != nullptr) {
    m_sink->onTopicVisibility(found->topic, visible);
  }
}

void Recording::tick(double wallSeconds) {
  if (!isOpen()) {
    return;
  }
  const Playback::Range range = m_playback.advance(wallSeconds);
  if (!range.empty() && !m_streamTopics.empty()) {
    m_source.forEach(range.from, range.to, m_streamTopics,
                     [this](const RawMessage& message) { apply(message); });
  }
  if (range.wrapped) {
    reload(m_playback.current());
  }
  if (m_sink != nullptr) {
    m_sink->onTick(m_playback.current());
  }
}

void Recording::seek(Time t) {
  if (!isOpen()) {
    return;
  }
  m_playback.seek(t);
  reload(m_playback.current());
  if (m_sink != nullptr) {
    m_sink->onTick(m_playback.current());
  }
}

void Recording::reload(Time t) {
  if (m_sink != nullptr) {
    m_sink->onSeek(t);
  }
  for (const std::string& topic : m_streamTopics) {
    std::optional<std::vector<std::byte>> latest =
        m_source.latestBefore(topic, t);
    if (!latest.has_value()) {
      continue;
    }
    const RecordingTopic* info = this->topic(topic);
    apply(RawMessage{.topic = topic,
                     .schemaName = info != nullptr ? info->schemaName : "",
                     .logTime = t,
                     .data = *latest});
  }
}

bool Recording::apply(const RawMessage& message) {
  const std::string topic(message.topic);
  try {
    dispatch(topic, message.schemaName, message.data);
    ++m_delivered;
    return true;
  } catch (const std::exception& error) {
    ++m_decodeErrors;
    if (m_warnedTopics.insert(topic).second) {
      spdlog::warn("Recording: {} at {}: {}", topic, message.logTime,
                   error.what());
    }
    return false;
  }
}

void Recording::dispatch(const std::string& topic, std::string_view schemaName,
                         std::span<const std::byte> data) {
  if (schemaName == kFrameTransformSchema) {
    m_tree.add(decodeFrameTransform(data));
    return;
  }
  if (schemaName == kPointCloudSchema) {
    PointCloud cloud = decodePointCloud(data);
    adoptRenderFrame(cloud.frameId);
    if (m_sink != nullptr) {
      m_sink->onPointCloud(topic, std::move(cloud));
    }
    return;
  }
  if (schemaName == kSceneUpdateSchema) {
    SceneUpdate update = decodeSceneUpdate(data);
    if (!update.entities.empty()) {
      adoptRenderFrame(update.entities.front().frameId);
    }
    if (m_sink != nullptr) {
      m_sink->onSceneUpdate(topic, std::move(update));
    }
    return;
  }
  if (schemaName == kCompressedImageSchema) {
    CompressedImage image = decodeCompressedImage(data);
    if (m_sink != nullptr) {
      m_sink->onImage(topic, std::move(image));
    }
    return;
  }
  if (schemaName == kImageAnnotationsSchema) {
    ImageAnnotations annotations = decodeImageAnnotations(data);
    if (m_sink != nullptr) {
      m_sink->onImageAnnotations(topic, std::move(annotations));
    }
    return;
  }
  if (schemaName == kCameraCalibrationSchema) {
    CameraCalibration calibration = decodeCameraCalibration(data);
    if (m_sink != nullptr) {
      m_sink->onCameraCalibration(topic, std::move(calibration));
    }
    return;
  }
  throw std::runtime_error("unsupported schema " + std::string(schemaName));
}

}  // namespace camelot

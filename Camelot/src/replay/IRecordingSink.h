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

#ifndef CAMELOT_SRC_REPLAY_IRECORDINGSINK_H_
#define CAMELOT_SRC_REPLAY_IRECORDINGSINK_H_

#include <string>

#include "Camelot/src/data/FoxgloveMessages.h"

namespace camelot {

// Receives the decoded messages of a Recording in log-time order. The
// message callbacks hand over the decoded struct by value so a sink can keep
// it without copying. The remaining callbacks frame the message stream: a
// seek (drop time-dependent state, the latest message of every topic
// follows), the end of a tick (the playback time to expire lifetimes
// against) and topic visibility changes.
class IRecordingSink {
 public:
  IRecordingSink() = default;
  IRecordingSink(const IRecordingSink&) = default;
  IRecordingSink& operator=(const IRecordingSink&) = default;
  IRecordingSink(IRecordingSink&&) = default;
  IRecordingSink& operator=(IRecordingSink&&) = default;
  virtual ~IRecordingSink() = default;

  virtual void onPointCloud(const std::string& topic, PointCloud cloud) = 0;
  virtual void onSceneUpdate(const std::string& topic, SceneUpdate update) = 0;
  virtual void onImage(const std::string& topic, CompressedImage image) = 0;
  virtual void onImageAnnotations(const std::string& topic,
                                  ImageAnnotations annotations) = 0;
  virtual void onCameraCalibration(const std::string& topic,
                                   CameraCalibration calibration) = 0;
  // Once per topic whose schema is not rendered.
  virtual void onUnsupported(const std::string& topic,
                             const std::string& schemaName) = 0;

  // The playback position jumped to `t`: forget every entity, cloud and
  // annotation; the latest message at or before `t` of every supported
  // topic is delivered right after this call.
  virtual void onSeek(Time t) { (void)t; }
  // End of a tick at playback time `now` (also after a seek). Lifetimes are
  // checked and deferred work (image decoding) happens here.
  virtual void onTick(Time now) { (void)now; }
  virtual void onTopicVisibility(const std::string& topic, bool visible) {
    (void)topic;
    (void)visible;
  }
};

}  // namespace camelot

#endif  // CAMELOT_SRC_REPLAY_IRECORDINGSINK_H_

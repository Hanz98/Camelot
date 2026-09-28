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

#ifndef CAMELOT_SRC_REPLAY_SCENEUPDATER_H_
#define CAMELOT_SRC_REPLAY_SCENEUPDATER_H_

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/renderer/LineDrawable.h"
#include "Avalon/src/renderer/MeshDrawable.h"
#include "Avalon/src/renderer/PointCloudDrawable.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/UiContext.h"
#include "Avalon/src/ui/VideoTexture.h"
#include "Camelot/src/data/FoxgloveMessages.h"
#include "Camelot/src/data/TransformTree.h"
#include "Camelot/src/replay/IRecordingSink.h"

namespace camelot {

// Turns the decoded messages of a Recording into Avalon drawables and keeps
// the Renderer in sync: one PointCloudDrawable per cloud topic, one mesh or
// line drawable per scene-entity primitive (keyed by topic and entity id,
// honouring deletions and lifetimes), one VideoTexture per camera with the
// image annotations of the same namespace drawn as overlays in pixel space.
// Everything is expressed in the render frame through the TransformTree.
//
// Boxes, spheres and line drawables are pooled: a retired drawable stays
// registered with the renderer, invisible, and is reused by the next entity,
// because Renderer::removeDrawable() waits for the device and scene updates
// arrive several times per second. clear() releases everything.
class SceneUpdater : public IRecordingSink {
 public:
  struct EntityKey {
    std::string topic;
    std::string id;
    auto operator<=>(const EntityKey&) const = default;
  };

  struct Entity {
    Time timestamp{0};
    // timestamp + lifetime, 0 when the entity never expires.
    Time expiresAt{0};
    std::string frameId;
    std::vector<std::shared_ptr<avalon::MeshDrawable>> boxes;    // pooled
    std::vector<std::shared_ptr<avalon::MeshDrawable>> spheres;  // pooled
    std::vector<std::shared_ptr<avalon::MeshDrawable>> arrows;   // unique
    std::vector<std::shared_ptr<avalon::LineDrawable>> lines;    // pooled
    uint32_t textCount{0};
    uint32_t unsupportedCount{0};
  };

  struct Cloud {
    std::shared_ptr<avalon::PointCloudDrawable> drawable;
    std::string frameId;
    uint64_t messages{0};
  };

  struct Camera {
    std::string imageTopic;
    std::string annotationTopic;
    std::shared_ptr<avalon::VideoTexture> texture;
    // The newest image of the tick; decoded and uploaded in onTick().
    std::optional<CompressedImage> pending;
    // The latest annotations, re-applied when the texture is recreated.
    std::optional<ImageAnnotations> annotations;
    std::vector<std::shared_ptr<avalon::LineDrawable>> lineOverlays;
    size_t linesUsed{0};
    std::vector<std::shared_ptr<avalon::PointCloudDrawable>> pointOverlays;
    size_t pointsUsed{0};
    uint64_t framesDecoded{0};
    uint64_t framesSkipped{0};
    uint32_t textCount{0};
    bool warnedDecode{false};
  };

  static constexpr float kLidarPointSize = 2.0F;
  static constexpr float kRadarPointSize = 6.0F;
  static constexpr uint32_t kCircleSegments = 32;

  // Throws std::runtime_error when any argument is null. The renderer and
  // the UI context are not owned and must outlive the updater (or clear()
  // must run before they go away).
  SceneUpdater(std::shared_ptr<avalon::Device> device,
               std::shared_ptr<avalon::VmaAllocatorWrapper> allocator,
               avalon::Renderer* renderer, const avalon::UiContext* ui);
  SceneUpdater(const SceneUpdater&) = delete;
  SceneUpdater& operator=(const SceneUpdater&) = delete;
  SceneUpdater(SceneUpdater&&) = delete;
  SceneUpdater& operator=(SceneUpdater&&) = delete;
  ~SceneUpdater() override;

  // The tree the poses are resolved through (not owned, may be null: every
  // frame is then drawn as the render frame).
  void setTransforms(const TransformTree* transforms) {
    m_transforms = transforms;
  }
  // The frame everything is drawn in. Empty adopts the frame of the first
  // entity or cloud that arrives.
  void setRenderFrame(std::string frame);
  [[nodiscard]] const std::string& renderFrame() const { return m_renderFrame; }

  // Removes every drawable and pre-pass from the renderer and forgets all
  // state; safe to call more than once.
  void clear();

  // IRecordingSink
  void onPointCloud(const std::string& topic, PointCloud cloud) override;
  void onSceneUpdate(const std::string& topic, SceneUpdate update) override;
  void onImage(const std::string& topic, CompressedImage image) override;
  void onImageAnnotations(const std::string& topic,
                          ImageAnnotations annotations) override;
  void onCameraCalibration(const std::string& topic,
                           CameraCalibration calibration) override;
  void onUnsupported(const std::string& topic,
                     const std::string& schemaName) override;
  void onSeek(Time t) override;
  void onTick(Time now) override;
  void onTopicVisibility(const std::string& topic, bool visible) override;

  [[nodiscard]] bool isTopicVisible(std::string_view topic) const;

  // Inspection (tests, the report line of the UI).
  [[nodiscard]] size_t entityCount() const { return m_entities.size(); }
  [[nodiscard]] const Entity* entity(std::string_view topic,
                                     std::string_view id) const;
  [[nodiscard]] size_t cloudCount() const { return m_clouds.size(); }
  [[nodiscard]] const Cloud* cloud(std::string_view topic) const;
  [[nodiscard]] size_t cameraCount() const { return m_cameras.size(); }
  // The camera of an image (or annotations) topic, by namespace.
  [[nodiscard]] const Camera* camera(std::string_view topic) const;
  [[nodiscard]] std::shared_ptr<avalon::VideoTexture> cameraTexture(
      std::string_view topic) const;
  // Text primitives of the live entities (counted, not drawn).
  [[nodiscard]] uint32_t textCount() const;
  // Drawables parked in the pools, invisible.
  [[nodiscard]] size_t pooledCount() const;
  [[nodiscard]] const std::vector<std::string>& unsupportedTopics() const {
    return m_unsupported;
  }
  [[nodiscard]] uint64_t calibrationCount() const { return m_calibrations; }

  // "/CAM_FRONT/image_rect_compressed" -> "/CAM_FRONT"; a topic without a
  // namespace is its own namespace.
  [[nodiscard]] static std::string cameraNamespace(std::string_view topic);
  // Proto3 default (all zero) colours read as "unspecified": opaque white.
  [[nodiscard]] static glm::vec4 toColor(const Color& color);
  [[nodiscard]] static glm::mat4 toMatrix(const Pose& pose);

 private:
  std::shared_ptr<avalon::Device> m_device;
  std::shared_ptr<avalon::VmaAllocatorWrapper> m_allocator;
  avalon::Renderer* m_renderer;   // not owned
  const avalon::UiContext* m_ui;  // not owned
  const TransformTree* m_transforms{nullptr};
  std::string m_renderFrame;

  std::map<EntityKey, Entity> m_entities;
  std::map<std::string, Cloud, std::less<>> m_clouds;
  std::map<std::string, Camera, std::less<>> m_cameras;  // by namespace
  std::map<std::string, bool, std::less<>> m_visible;
  std::vector<std::string> m_unsupported;
  std::set<std::string> m_warnedFrames;
  uint64_t m_calibrations{0};

  std::vector<std::shared_ptr<avalon::MeshDrawable>> m_freeBoxes;
  std::vector<std::shared_ptr<avalon::MeshDrawable>> m_freeSpheres;
  std::vector<std::shared_ptr<avalon::LineDrawable>> m_freeLines;

  // Pose of `frameId` at `t` in the render frame (identity with one warning
  // when the frames are not connected).
  [[nodiscard]] glm::mat4 frameTransform(const std::string& frameId, Time t);
  void adoptRenderFrame(const std::string& frameId);

  void applyDeletion(const std::string& topic,
                     const SceneEntityDeletion& deletion);
  void buildEntity(const std::string& topic, const SceneEntity& source);
  // Parks the pooled drawables of `entity` and drops the arrows.
  void retireEntity(Entity& entity);
  std::shared_ptr<avalon::MeshDrawable> takeBox();
  std::shared_ptr<avalon::MeshDrawable> takeSphere();
  std::shared_ptr<avalon::LineDrawable> takeLine();
  void addLine(Entity& entity, const LinePrimitive& line,
               const glm::mat4& frame, bool visible);
  void addArrow(Entity& entity, const ArrowPrimitive& arrow,
                const glm::mat4& frame, bool visible);
  void setEntityVisible(const Entity& entity, bool visible);

  Camera& cameraFor(const std::string& topic);
  void decodePendingImage(Camera& camera);
  void ensureTexture(Camera& camera, uint32_t width, uint32_t height);
  void applyAnnotations(Camera& camera);
  void hideOverlays(Camera& camera);
  std::shared_ptr<avalon::LineDrawable> takeOverlayLine(Camera& camera);
  std::shared_ptr<avalon::PointCloudDrawable> takeOverlayPoints(Camera& camera);
  void addPointsAnnotation(Camera& camera, const PointsAnnotation& points,
                           bool visible);
  void addCircleAnnotation(Camera& camera, const CircleAnnotation& circle,
                           bool visible);
};

}  // namespace camelot

#endif  // CAMELOT_SRC_REPLAY_SCENEUPDATER_H_

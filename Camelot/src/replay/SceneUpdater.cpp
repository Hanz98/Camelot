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

#include "Camelot/src/replay/SceneUpdater.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/geometry/Shapes.h"
#include "Avalon/src/geometry/Vertex.h"
#include "Camelot/src/data/JpegDecoder.h"

namespace camelot {

namespace {

constexpr glm::vec4 kDefaultColor{1.0F, 1.0F, 1.0F, 1.0F};
constexpr glm::vec4 kIntensityLow{0.15F, 0.35F, 1.0F, 1.0F};
constexpr glm::vec4 kIntensityHigh{1.0F, 0.9F, 0.1F, 1.0F};
constexpr float kMinAnnotationPointSize = 4.0F;

glm::vec3 toVec3(const Vec3& v) {
  return {static_cast<float>(v.x), static_cast<float>(v.y),
          static_cast<float>(v.z)};
}

glm::vec3 toVec3(const Vec2& v) {
  return {static_cast<float>(v.x), static_cast<float>(v.y), 0.0F};
}

bool containsRadar(std::string_view topic) {
  std::string lower(topic);
  std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return lower.find("radar") != std::string::npos;
}

// Point colours from the intensity range of the cloud: blue for the lowest
// intensity, yellow for the highest (a single value is drawn mid-way).
std::vector<avalon::PointVertex> colourByIntensity(
    const std::vector<glm::vec4>& points) {
  float low = std::numeric_limits<float>::max();
  float high = std::numeric_limits<float>::lowest();
  for (const glm::vec4& p : points) {
    low = std::min(low, p.w);
    high = std::max(high, p.w);
  }
  const float span = high - low;
  std::vector<avalon::PointVertex> out;
  out.reserve(points.size());
  for (const glm::vec4& p : points) {
    const float t = span > 0.0F ? (p.w - low) / span : 0.5F;
    out.push_back({.position = glm::vec3(p),
                   .color = glm::mix(kIntensityLow, kIntensityHigh, t)});
  }
  return out;
}

// The point sequence of a line primitive: its indices when present (out of
// range ones dropped), else every point in order.
std::vector<uint32_t> lineSequence(const LinePrimitive& line) {
  std::vector<uint32_t> sequence;
  const auto count = static_cast<uint32_t>(line.points.size());
  if (line.indices.empty()) {
    sequence.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
      sequence[i] = i;
    }
    return sequence;
  }
  sequence.reserve(line.indices.size());
  for (const uint32_t index : line.indices) {
    if (index < count) {
      sequence.push_back(index);
    }
  }
  return sequence;
}

}  // namespace

SceneUpdater::SceneUpdater(
    std::shared_ptr<avalon::Device> device,
    std::shared_ptr<avalon::VmaAllocatorWrapper> allocator,
    avalon::Renderer* renderer, const avalon::UiContext* ui)
    : m_device(std::move(device)),
      m_allocator(std::move(allocator)),
      m_renderer(renderer),
      m_ui(ui) {
  if (m_device == nullptr || m_allocator == nullptr || m_renderer == nullptr ||
      m_ui == nullptr) {
    spdlog::error("SceneUpdater: device, allocator, renderer or UI is null.");
    throw std::runtime_error(
        "SceneUpdater: device, allocator, renderer or UI is null.");
  }
}

SceneUpdater::~SceneUpdater() { clear(); }

std::string SceneUpdater::cameraNamespace(std::string_view topic) {
  const size_t slash = topic.rfind('/');
  if (slash == std::string_view::npos || slash == 0) {
    return std::string(topic);
  }
  return std::string(topic.substr(0, slash));
}

glm::vec4 SceneUpdater::toColor(const Color& color) {
  if (color.r == 0.0F && color.g == 0.0F && color.b == 0.0F &&
      color.a == 0.0F) {
    return kDefaultColor;
  }
  return {color.r, color.g, color.b, color.a};
}

glm::mat4 SceneUpdater::toMatrix(const Pose& pose) {
  return TransformTree::toMatrix(TransformTree::Sample{
      .time = 0, .translation = pose.position, .rotation = pose.orientation});
}

void SceneUpdater::setRenderFrame(std::string frame) {
  m_renderFrame = std::move(frame);
}

void SceneUpdater::adoptRenderFrame(const std::string& frameId) {
  if (m_renderFrame.empty() && !frameId.empty()) {
    m_renderFrame = frameId;
    spdlog::info("SceneUpdater: rendering in frame '{}'", frameId);
  }
}

glm::mat4 SceneUpdater::frameTransform(const std::string& frameId, Time t) {
  adoptRenderFrame(frameId);
  if (frameId.empty() || frameId == m_renderFrame) {
    return {1.0F};
  }
  if (m_transforms != nullptr) {
    const std::optional<glm::mat4> found =
        m_transforms->lookup(m_renderFrame, frameId, t);
    if (found.has_value()) {
      return *found;
    }
  }
  if (m_warnedFrames.insert(frameId).second) {
    spdlog::warn(
        "SceneUpdater: no transform from '{}' to '{}'; drawing in place.",
        frameId, m_renderFrame);
  }
  return {1.0F};
}

void SceneUpdater::clear() {
  for (auto& [key, entity] : m_entities) {
    retireEntity(entity);
  }
  m_entities.clear();
  for (const auto& box : m_freeBoxes) {
    m_renderer->removeDrawable(box);
  }
  for (const auto& sphere : m_freeSpheres) {
    m_renderer->removeDrawable(sphere);
  }
  for (const auto& line : m_freeLines) {
    m_renderer->removeDrawable(line);
  }
  m_freeBoxes.clear();
  m_freeSpheres.clear();
  m_freeLines.clear();
  for (auto& [topic, cloud] : m_clouds) {
    m_renderer->removeDrawable(cloud.drawable);
  }
  m_clouds.clear();
  for (auto& [ns, camera] : m_cameras) {
    if (camera.texture != nullptr) {
      m_renderer->removePrePass(camera.texture);
    }
  }
  m_cameras.clear();
  m_visible.clear();
  m_unsupported.clear();
  m_warnedFrames.clear();
  m_calibrations = 0;
}

// ---------------------------------------------------------------- topics --

bool SceneUpdater::isTopicVisible(std::string_view topic) const {
  const auto found = m_visible.find(topic);
  return found == m_visible.end() || found->second;
}

void SceneUpdater::onTopicVisibility(const std::string& topic, bool visible) {
  m_visible[topic] = visible;
  for (const auto& [key, entity] : m_entities) {
    if (key.topic == topic) {
      setEntityVisible(entity, visible);
    }
  }
  const auto cloud = m_clouds.find(topic);
  if (cloud != m_clouds.end()) {
    cloud->second.drawable->setVisible(visible);
  }
  const auto camera = m_cameras.find(cameraNamespace(topic));
  if (camera != m_cameras.end() && camera->second.annotationTopic == topic) {
    for (size_t i = 0; i < camera->second.linesUsed; ++i) {
      camera->second.lineOverlays[i]->setVisible(visible);
    }
    for (size_t i = 0; i < camera->second.pointsUsed; ++i) {
      camera->second.pointOverlays[i]->setVisible(visible);
    }
  }
}

void SceneUpdater::onUnsupported(const std::string& topic,
                                 const std::string& schemaName) {
  if (std::ranges::find(m_unsupported, topic) == m_unsupported.end()) {
    m_unsupported.push_back(topic);
    spdlog::warn("SceneUpdater: topic {} ({}) is not rendered.", topic,
                 schemaName);
  }
}

void SceneUpdater::onCameraCalibration(const std::string& topic,
                                       CameraCalibration calibration) {
  (void)topic;
  (void)calibration;
  ++m_calibrations;
}

// ---------------------------------------------------------- point clouds --

void SceneUpdater::onPointCloud(const std::string& topic, PointCloud cloud) {
  auto found = m_clouds.find(topic);
  if (found == m_clouds.end()) {
    Cloud entry;
    entry.drawable =
        std::make_shared<avalon::PointCloudDrawable>(m_device, m_allocator);
    entry.drawable->setPointSize(containsRadar(topic) ? kRadarPointSize
                                                      : kLidarPointSize);
    entry.drawable->setVisible(isTopicVisible(topic));
    m_renderer->addDrawable(entry.drawable);
    found = m_clouds.emplace(topic, std::move(entry)).first;
  }
  Cloud& entry = found->second;
  entry.frameId = cloud.frameId;
  ++entry.messages;
  const std::vector<avalon::PointVertex> vertices =
      colourByIntensity(cloud.positionsAndIntensity());
  entry.drawable->setPoints(vertices);
  const glm::mat4 frame = frameTransform(cloud.frameId, cloud.timestamp);
  entry.drawable->setTransform(frame * toMatrix(cloud.pose));
}

const SceneUpdater::Cloud* SceneUpdater::cloud(std::string_view topic) const {
  const auto found = m_clouds.find(topic);
  return found == m_clouds.end() ? nullptr : &found->second;
}

// -------------------------------------------------------- scene entities --

void SceneUpdater::onSceneUpdate(const std::string& topic, SceneUpdate update) {
  for (const SceneEntityDeletion& deletion : update.deletions) {
    applyDeletion(topic, deletion);
  }
  for (const SceneEntity& entity : update.entities) {
    buildEntity(topic, entity);
  }
}

void SceneUpdater::applyDeletion(const std::string& topic,
                                 const SceneEntityDeletion& deletion) {
  for (auto it = m_entities.begin(); it != m_entities.end();) {
    const bool matches =
        it->first.topic == topic &&
        (deletion.type == DeletionType::kAll || it->first.id == deletion.id);
    if (matches) {
      retireEntity(it->second);
      it = m_entities.erase(it);
    } else {
      ++it;
    }
  }
}

void SceneUpdater::retireEntity(Entity& entity) {
  for (auto& box : entity.boxes) {
    box->setVisible(false);
    m_freeBoxes.push_back(std::move(box));
  }
  for (auto& sphere : entity.spheres) {
    sphere->setVisible(false);
    m_freeSpheres.push_back(std::move(sphere));
  }
  for (auto& line : entity.lines) {
    line->setVisible(false);
    m_freeLines.push_back(std::move(line));
  }
  for (const auto& arrow : entity.arrows) {
    m_renderer->removeDrawable(arrow);
  }
  entity.boxes.clear();
  entity.spheres.clear();
  entity.lines.clear();
  entity.arrows.clear();
}

std::shared_ptr<avalon::MeshDrawable> SceneUpdater::takeBox() {
  if (!m_freeBoxes.empty()) {
    auto box = std::move(m_freeBoxes.back());
    m_freeBoxes.pop_back();
    return box;
  }
  auto box = std::make_shared<avalon::MeshDrawable>(m_device, m_allocator,
                                                    avalon::shapes::box());
  m_renderer->addDrawable(box);
  return box;
}

std::shared_ptr<avalon::MeshDrawable> SceneUpdater::takeSphere() {
  if (!m_freeSpheres.empty()) {
    auto sphere = std::move(m_freeSpheres.back());
    m_freeSpheres.pop_back();
    return sphere;
  }
  auto sphere = std::make_shared<avalon::MeshDrawable>(
      m_device, m_allocator, avalon::shapes::sphere());
  m_renderer->addDrawable(sphere);
  return sphere;
}

std::shared_ptr<avalon::LineDrawable> SceneUpdater::takeLine() {
  if (!m_freeLines.empty()) {
    auto line = std::move(m_freeLines.back());
    m_freeLines.pop_back();
    return line;
  }
  auto line = std::make_shared<avalon::LineDrawable>(m_device, m_allocator);
  m_renderer->addDrawable(line);
  return line;
}

void SceneUpdater::buildEntity(const std::string& topic,
                               const SceneEntity& source) {
  const EntityKey key{.topic = topic, .id = source.id};
  Entity& entity = m_entities[key];
  retireEntity(entity);

  entity.timestamp = source.timestamp;
  entity.expiresAt =
      source.lifetime != 0 ? source.timestamp + source.lifetime : 0;
  entity.frameId = source.frameId;
  entity.textCount = static_cast<uint32_t>(source.texts.size());
  entity.unsupportedCount =
      source.cylinderCount + source.triangleCount + source.modelCount;
  const bool visible = isTopicVisible(topic);
  const glm::mat4 frame = frameTransform(source.frameId, source.timestamp);

  for (const CubePrimitive& cube : source.cubes) {
    auto box = takeBox();
    box->setTransform(
        glm::scale(frame * toMatrix(cube.pose), toVec3(cube.size)));
    box->setColor(toColor(cube.color));
    box->setVisible(visible);
    entity.boxes.push_back(std::move(box));
  }
  for (const SpherePrimitive& sphere : source.spheres) {
    auto shape = takeSphere();
    // shapes::sphere() has a diameter of one; size holds the diameters.
    shape->setTransform(
        glm::scale(frame * toMatrix(sphere.pose), toVec3(sphere.size)));
    shape->setColor(toColor(sphere.color));
    shape->setVisible(visible);
    entity.spheres.push_back(std::move(shape));
  }
  for (const ArrowPrimitive& arrow : source.arrows) {
    addArrow(entity, arrow, frame, visible);
  }
  for (const LinePrimitive& line : source.lines) {
    addLine(entity, line, frame, visible);
  }
}

void SceneUpdater::addArrow(Entity& entity, const ArrowPrimitive& arrow,
                            const glm::mat4& frame, bool visible) {
  // Foxglove arrows point along +X of their pose: a shaft from the origin
  // and a wider, shorter head after it. shapes::cylinder() runs along +Z
  // centred on the origin, so each part is rotated onto +X and shifted.
  const auto shaftLength = static_cast<float>(arrow.shaftLength);
  const auto headLength = static_cast<float>(arrow.headLength);
  const glm::mat4 zToX = glm::rotate(
      glm::mat4(1.0F), std::numbers::pi_v<float> / 2.0F, {0.0F, 1.0F, 0.0F});
  avalon::MeshData mesh = avalon::shapes::cylinder(
      static_cast<float>(arrow.shaftDiameter) / 2.0F, shaftLength);
  avalon::shapes::applyTransform(
      mesh,
      glm::translate(glm::mat4(1.0F), {shaftLength / 2.0F, 0.0F, 0.0F}) * zToX);
  avalon::MeshData head = avalon::shapes::cylinder(
      static_cast<float>(arrow.headDiameter) / 2.0F, headLength);
  avalon::shapes::applyTransform(
      head, glm::translate(glm::mat4(1.0F),
                           {shaftLength + headLength / 2.0F, 0.0F, 0.0F}) *
                zToX);
  avalon::shapes::append(mesh, head);
  auto drawable =
      std::make_shared<avalon::MeshDrawable>(m_device, m_allocator, mesh);
  drawable->setTransform(frame * toMatrix(arrow.pose));
  drawable->setColor(toColor(arrow.color));
  drawable->setVisible(visible);
  m_renderer->addDrawable(drawable);
  entity.arrows.push_back(std::move(drawable));
}

void SceneUpdater::addLine(Entity& entity, const LinePrimitive& line,
                           const glm::mat4& frame, bool visible) {
  const bool perPoint = line.colors.size() == line.points.size();
  const glm::vec4 color = toColor(line.color);
  std::vector<avalon::PointVertex> vertices;
  for (const uint32_t index : lineSequence(line)) {
    vertices.push_back(
        {.position = toVec3(line.points[index]),
         .color = perPoint ? toColor(line.colors[index]) : color});
  }
  auto drawable = takeLine();
  if (line.type == LineType::kLineList) {
    if (vertices.size() % 2 != 0) {
      vertices.pop_back();
    }
    drawable->setLines(vertices);
  } else {
    drawable->setStrip(vertices, line.type == LineType::kLineLoop);
  }
  // Thickness is in pixels only for scale-invariant lines; a world-space
  // thickness has no equivalent in the line pipeline.
  drawable->setLineWidth(line.scaleInvariant
                             ? static_cast<float>(line.thickness)
                             : avalon::LineDrawable::kDefaultLineWidth);
  drawable->setTransform(frame * toMatrix(line.pose));
  drawable->setTint(glm::vec4(1.0F));
  drawable->setVisible(visible);
  entity.lines.push_back(std::move(drawable));
}

void SceneUpdater::setEntityVisible(const Entity& entity, bool visible) {
  for (const auto& box : entity.boxes) {
    box->setVisible(visible);
  }
  for (const auto& sphere : entity.spheres) {
    sphere->setVisible(visible);
  }
  for (const auto& arrow : entity.arrows) {
    arrow->setVisible(visible);
  }
  for (const auto& line : entity.lines) {
    line->setVisible(visible);
  }
}

const SceneUpdater::Entity* SceneUpdater::entity(std::string_view topic,
                                                 std::string_view id) const {
  const auto found = m_entities.find(
      EntityKey{.topic = std::string(topic), .id = std::string(id)});
  return found == m_entities.end() ? nullptr : &found->second;
}

uint32_t SceneUpdater::textCount() const {
  uint32_t count = 0;
  for (const auto& [key, entity] : m_entities) {
    count += entity.textCount;
  }
  for (const auto& [ns, camera] : m_cameras) {
    count += camera.textCount;
  }
  return count;
}

size_t SceneUpdater::pooledCount() const {
  return m_freeBoxes.size() + m_freeSpheres.size() + m_freeLines.size();
}

// --------------------------------------------------------------- cameras --

SceneUpdater::Camera& SceneUpdater::cameraFor(const std::string& topic) {
  return m_cameras[cameraNamespace(topic)];
}

const SceneUpdater::Camera* SceneUpdater::camera(std::string_view topic) const {
  const auto found = m_cameras.find(cameraNamespace(topic));
  return found == m_cameras.end() ? nullptr : &found->second;
}

std::shared_ptr<avalon::VideoTexture> SceneUpdater::cameraTexture(
    std::string_view topic) const {
  const Camera* found = camera(topic);
  return found == nullptr ? nullptr : found->texture;
}

void SceneUpdater::onImage(const std::string& topic, CompressedImage image) {
  Camera& camera = cameraFor(topic);
  camera.imageTopic = topic;
  if (camera.pending.has_value()) {
    ++camera.framesSkipped;  // only the newest frame of a tick is decoded
  }
  camera.pending = std::move(image);
}

void SceneUpdater::ensureTexture(Camera& camera, uint32_t width,
                                 uint32_t height) {
  if (camera.texture != nullptr && camera.texture->width() == width &&
      camera.texture->height() == height) {
    return;
  }
  if (camera.texture != nullptr) {
    m_renderer->removePrePass(camera.texture);
  }
  camera.texture = std::make_shared<avalon::VideoTexture>(
      m_device, m_allocator, m_ui, width, height,
      avalon::Renderer::kFramesInFlight);
  m_renderer->addPrePass(camera.texture);
  // The overlay pools follow the texture.
  for (const auto& line : camera.lineOverlays) {
    camera.texture->addOverlay(line);
  }
  for (const auto& points : camera.pointOverlays) {
    camera.texture->addOverlay(points);
  }
  spdlog::info("SceneUpdater: {} is {}x{}", camera.imageTopic, width, height);
}

void SceneUpdater::decodePendingImage(Camera& camera) {
  if (!camera.pending.has_value()) {
    return;
  }
  const CompressedImage image = std::move(*camera.pending);
  camera.pending.reset();
  try {
    if (!JpegDecoder::looksLikeJpeg(image.data)) {
      throw std::runtime_error("format '" + image.format +
                               "' is not JPEG (only JPEG is decoded)");
    }
    const DecodedImage decoded = JpegDecoder::decodeJpeg(image.data);
    ensureTexture(camera, decoded.width, decoded.height);
    camera.texture->upload(decoded.rgba);
    ++camera.framesDecoded;
  } catch (const std::exception& error) {
    if (!camera.warnedDecode) {
      camera.warnedDecode = true;
      spdlog::warn("SceneUpdater: {}: {}", camera.imageTopic, error.what());
    }
  }
}

void SceneUpdater::onImageAnnotations(const std::string& topic,
                                      ImageAnnotations annotations) {
  Camera& camera = cameraFor(topic);
  camera.annotationTopic = topic;
  camera.annotations = std::move(annotations);
  applyAnnotations(camera);
}

void SceneUpdater::hideOverlays(Camera& camera) {
  for (const auto& line : camera.lineOverlays) {
    line->setVisible(false);
  }
  for (const auto& points : camera.pointOverlays) {
    points->setVisible(false);
  }
  camera.linesUsed = 0;
  camera.pointsUsed = 0;
}

std::shared_ptr<avalon::LineDrawable> SceneUpdater::takeOverlayLine(
    Camera& camera) {
  if (camera.linesUsed == camera.lineOverlays.size()) {
    auto line = std::make_shared<avalon::LineDrawable>(m_device, m_allocator);
    if (camera.texture != nullptr) {
      camera.texture->addOverlay(line);
    }
    camera.lineOverlays.push_back(std::move(line));
  }
  return camera.lineOverlays[camera.linesUsed++];
}

std::shared_ptr<avalon::PointCloudDrawable> SceneUpdater::takeOverlayPoints(
    Camera& camera) {
  if (camera.pointsUsed == camera.pointOverlays.size()) {
    auto points =
        std::make_shared<avalon::PointCloudDrawable>(m_device, m_allocator);
    if (camera.texture != nullptr) {
      camera.texture->addOverlay(points);
    }
    camera.pointOverlays.push_back(std::move(points));
  }
  return camera.pointOverlays[camera.pointsUsed++];
}

void SceneUpdater::applyAnnotations(Camera& camera) {
  hideOverlays(camera);
  if (!camera.annotations.has_value()) {
    camera.textCount = 0;
    return;
  }
  const ImageAnnotations& annotations = *camera.annotations;
  const bool visible = isTopicVisible(camera.annotationTopic);
  for (const PointsAnnotation& points : annotations.points) {
    addPointsAnnotation(camera, points, visible);
  }
  for (const CircleAnnotation& circle : annotations.circles) {
    addCircleAnnotation(camera, circle, visible);
  }
  camera.textCount = static_cast<uint32_t>(annotations.texts.size());
}

void SceneUpdater::addPointsAnnotation(Camera& camera,
                                       const PointsAnnotation& points,
                                       bool visible) {
  if (points.type == PointsType::kUnknown || points.points.empty()) {
    return;
  }
  const bool perPoint = points.outlineColors.size() == points.points.size();
  const glm::vec4 color = toColor(points.outlineColor);
  std::vector<avalon::PointVertex> vertices;
  vertices.reserve(points.points.size());
  for (size_t i = 0; i < points.points.size(); ++i) {
    vertices.push_back(
        {.position = toVec3(points.points[i]),
         .color = perPoint ? toColor(points.outlineColors[i]) : color});
  }
  const auto thickness = static_cast<float>(points.thickness);
  if (points.type == PointsType::kPoints) {
    auto drawable = takeOverlayPoints(camera);
    drawable->setPoints(vertices);
    drawable->setPointSize(std::max(kMinAnnotationPointSize, 2.0F * thickness));
    drawable->setVisible(visible);
    return;
  }
  auto drawable = takeOverlayLine(camera);
  if (points.type == PointsType::kLineList) {
    if (vertices.size() % 2 != 0) {
      vertices.pop_back();
    }
    drawable->setLines(vertices);
  } else {
    drawable->setStrip(vertices, points.type == PointsType::kLineLoop);
  }
  drawable->setLineWidth(
      std::max(avalon::LineDrawable::kDefaultLineWidth, thickness));
  drawable->setVisible(visible);
}

void SceneUpdater::addCircleAnnotation(Camera& camera,
                                       const CircleAnnotation& circle,
                                       bool visible) {
  const auto radius = static_cast<float>(circle.diameter) / 2.0F;
  const glm::vec3 centre = toVec3(circle.position);
  const glm::vec4 color = toColor(circle.outlineColor);
  std::vector<avalon::PointVertex> vertices;
  vertices.reserve(kCircleSegments);
  for (uint32_t i = 0; i < kCircleSegments; ++i) {
    const float angle = 2.0F * std::numbers::pi_v<float> *
                        static_cast<float>(i) /
                        static_cast<float>(kCircleSegments);
    vertices.push_back(
        {.position = centre + glm::vec3(std::cos(angle) * radius,
                                        std::sin(angle) * radius, 0.0F),
         .color = color});
  }
  auto drawable = takeOverlayLine(camera);
  drawable->setStrip(vertices, true);
  drawable->setLineWidth(std::max(avalon::LineDrawable::kDefaultLineWidth,
                                  static_cast<float>(circle.thickness)));
  drawable->setVisible(visible);
}

// ------------------------------------------------------------ tick/seek --

void SceneUpdater::onSeek(Time t) {
  (void)t;
  for (auto& [key, entity] : m_entities) {
    retireEntity(entity);
  }
  m_entities.clear();
  for (auto& [topic, cloud] : m_clouds) {
    cloud.drawable->setPoints({});
  }
  for (auto& [ns, camera] : m_cameras) {
    camera.pending.reset();
    camera.annotations.reset();
    applyAnnotations(camera);
  }
}

void SceneUpdater::onTick(Time now) {
  for (auto it = m_entities.begin(); it != m_entities.end();) {
    if (it->second.expiresAt != 0 && now >= it->second.expiresAt) {
      retireEntity(it->second);
      it = m_entities.erase(it);
    } else {
      ++it;
    }
  }
  for (auto& [ns, camera] : m_cameras) {
    decodePendingImage(camera);
  }
}

}  // namespace camelot

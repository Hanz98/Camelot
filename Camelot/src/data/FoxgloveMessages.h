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

#ifndef CAMELOT_SRC_DATA_FOXGLOVEMESSAGES_H_
#define CAMELOT_SRC_DATA_FOXGLOVEMESSAGES_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

// Plain structs mirroring the Foxglove message schemas that Camelot renders
// (the vendored .proto files under Camelot/src/data/schemas/ are the
// reference). Defaults follow proto3: a field that is absent on the wire
// reads as zero, so an omitted Color is transparent black and an omitted
// Quat is all zeros (TransformTree treats a zero quaternion as identity).

namespace camelot {

// Nanoseconds since the Unix epoch (google.protobuf.Timestamp flattened).
using Time = uint64_t;

// Nanoseconds (google.protobuf.Duration flattened).
using Duration = uint64_t;

struct Vec2 {
  double x{0.0};
  double y{0.0};
};

struct Vec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct Quat {
  double x{0.0};
  double y{0.0};
  double z{0.0};
  double w{0.0};
};

struct Pose {
  Vec3 position;
  Quat orientation;
};

// Channels in 0..1.
struct Color {
  float r{0.0F};
  float g{0.0F};
  float b{0.0F};
  float a{0.0F};
};

// foxglove.PackedElementField.NumericType.
enum class NumericType : uint8_t {
  kUnknown = 0,
  kUint8 = 1,
  kInt8 = 2,
  kUint16 = 3,
  kInt16 = 4,
  kUint32 = 5,
  kInt32 = 6,
  kFloat32 = 7,
  kFloat64 = 8,
};

struct PackedField {
  std::string name;
  uint32_t offset{0};
  NumericType type{NumericType::kUnknown};
};

struct PointCloud {
  Time timestamp{0};
  std::string frameId;
  Pose pose;
  uint32_t pointStride{0};
  std::vector<PackedField> fields;
  std::vector<std::byte> data;

  // Number of points in `data` (0 when the stride is 0).
  [[nodiscard]] size_t pointCount() const;
  // One (x, y, z, intensity) per point, resolved by field name and numeric
  // type. A missing coordinate or intensity reads as 0. Throws
  // std::runtime_error when a field lies outside the point stride.
  [[nodiscard]] std::vector<glm::vec4> positionsAndIntensity() const;
};

struct CompressedImage {
  Time timestamp{0};
  std::string frameId;
  std::vector<std::byte> data;
  std::string format;
};

struct FrameTransform {
  Time timestamp{0};
  std::string parentFrameId;
  std::string childFrameId;
  Vec3 translation;
  Quat rotation;
};

struct CubePrimitive {
  Pose pose;
  Vec3 size;
  Color color;
};

struct SpherePrimitive {
  Pose pose;
  Vec3 size;
  Color color;
};

// foxglove.LinePrimitive.Type.
enum class LineType : uint8_t {
  kLineStrip = 0,
  kLineLoop = 1,
  kLineList = 2,
};

struct LinePrimitive {
  LineType type{LineType::kLineStrip};
  Pose pose;
  double thickness{0.0};
  bool scaleInvariant{false};
  std::vector<Vec3> points;
  Color color;
  std::vector<Color> colors;
  std::vector<uint32_t> indices;
};

struct ArrowPrimitive {
  Pose pose;
  double shaftLength{0.0};
  double shaftDiameter{0.0};
  double headLength{0.0};
  double headDiameter{0.0};
  Color color;
};

struct TextPrimitive {
  Pose pose;
  bool billboard{false};
  double fontSize{0.0};
  bool scaleInvariant{false};
  Color color;
  std::string text;
};

struct SceneEntity {
  Time timestamp{0};
  std::string frameId;
  std::string id;
  Duration lifetime{0};
  bool frameLocked{false};
  std::vector<ArrowPrimitive> arrows;
  std::vector<CubePrimitive> cubes;
  std::vector<SpherePrimitive> spheres;
  std::vector<LinePrimitive> lines;
  std::vector<TextPrimitive> texts;
  // Primitives that are decoded only by count (not rendered).
  uint32_t cylinderCount{0};
  uint32_t triangleCount{0};
  uint32_t modelCount{0};
};

// foxglove.SceneEntityDeletion.Type.
enum class DeletionType : uint8_t {
  kMatchingId = 0,
  kAll = 1,
};

struct SceneEntityDeletion {
  Time timestamp{0};
  DeletionType type{DeletionType::kMatchingId};
  std::string id;
};

struct SceneUpdate {
  std::vector<SceneEntityDeletion> deletions;
  std::vector<SceneEntity> entities;
};

struct CircleAnnotation {
  Time timestamp{0};
  Vec2 position;
  double diameter{0.0};
  double thickness{0.0};
  Color fillColor;
  Color outlineColor;
};

// foxglove.PointsAnnotation.Type.
enum class PointsType : uint8_t {
  kUnknown = 0,
  kPoints = 1,
  kLineLoop = 2,
  kLineStrip = 3,
  kLineList = 4,
};

struct PointsAnnotation {
  Time timestamp{0};
  PointsType type{PointsType::kUnknown};
  std::vector<Vec2> points;
  Color outlineColor;
  std::vector<Color> outlineColors;
  Color fillColor;
  double thickness{0.0};
};

struct TextAnnotation {
  Time timestamp{0};
  Vec2 position;
  std::string text;
  double fontSize{0.0};
  Color textColor;
  Color backgroundColor;
};

struct ImageAnnotations {
  // Optional in the schema; 0 when absent.
  Time timestamp{0};
  std::vector<CircleAnnotation> circles;
  std::vector<PointsAnnotation> points;
  std::vector<TextAnnotation> texts;
};

struct CameraCalibration {
  static constexpr size_t kIntrinsicSize = 9;
  static constexpr size_t kProjectionSize = 12;

  Time timestamp{0};
  std::string frameId;
  uint32_t width{0};
  uint32_t height{0};
  std::string distortionModel;
  std::vector<double> D;
  std::array<double, kIntrinsicSize> K{};
  std::array<double, kIntrinsicSize> R{};
  std::array<double, kProjectionSize> P{};
};

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_FOXGLOVEMESSAGES_H_

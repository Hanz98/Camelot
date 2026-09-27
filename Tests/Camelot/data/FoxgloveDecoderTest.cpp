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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Camelot/src/data/FoxgloveDecoder.h"
#include "Camelot/src/data/FoxgloveMessages.h"

namespace camelot {

namespace {

constexpr uint64_t kNanosPerSecond = 1'000'000'000ULL;

// A minimal protobuf writer for the tests: it encodes the wire format the
// decoders must read, field numbers taken from the vendored schemas.
class ProtoWriter {
 public:
  void varint(uint32_t number, uint64_t value) {
    tag(number, 0);
    writeVarint(value);
  }
  void boolean(uint32_t number, bool value) { varint(number, value ? 1 : 0); }
  void fixed32(uint32_t number, uint32_t value) {
    tag(number, 5);
    raw(&value, sizeof(value));
  }
  void doubleField(uint32_t number, double value) {
    tag(number, 1);
    raw(&value, sizeof(value));
  }
  void string(uint32_t number, std::string_view value) {
    tag(number, 2);
    writeVarint(value.size());
    raw(value.data(), value.size());
  }
  void bytes(uint32_t number, std::span<const std::byte> value) {
    tag(number, 2);
    writeVarint(value.size());
    raw(value.data(), value.size());
  }
  void message(uint32_t number, const ProtoWriter& nested) {
    bytes(number, nested.data());
  }
  void packedDoubles(uint32_t number, std::span<const double> values) {
    tag(number, 2);
    writeVarint(values.size() * sizeof(double));
    raw(values.data(), values.size() * sizeof(double));
  }
  void packedFixed32(uint32_t number, std::span<const uint32_t> values) {
    tag(number, 2);
    writeVarint(values.size() * sizeof(uint32_t));
    raw(values.data(), values.size() * sizeof(uint32_t));
  }

  // Foxglove building blocks.
  void timestamp(uint32_t number, Time t) {
    ProtoWriter w;
    w.varint(1, t / kNanosPerSecond);
    w.varint(2, t % kNanosPerSecond);
    message(number, w);
  }
  void vec2(uint32_t number, double x, double y) {
    ProtoWriter w;
    w.doubleField(1, x);
    w.doubleField(2, y);
    message(number, w);
  }
  void vec3(uint32_t number, double x, double y, double z) {
    ProtoWriter w;
    w.doubleField(1, x);
    w.doubleField(2, y);
    w.doubleField(3, z);
    message(number, w);
  }
  void quat(uint32_t number, double x, double y, double z, double w) {
    ProtoWriter q;
    q.doubleField(1, x);
    q.doubleField(2, y);
    q.doubleField(3, z);
    q.doubleField(4, w);
    message(number, q);
  }
  void pose(uint32_t number, double x, double y, double z) {
    ProtoWriter p;
    p.vec3(1, x, y, z);
    p.quat(2, 0.0, 0.0, 0.0, 1.0);
    message(number, p);
  }
  void color(uint32_t number, double r, double g, double b, double a) {
    ProtoWriter c;
    c.doubleField(1, r);
    c.doubleField(2, g);
    c.doubleField(3, b);
    c.doubleField(4, a);
    message(number, c);
  }

  [[nodiscard]] std::span<const std::byte> data() const { return m_out; }

 private:
  std::vector<std::byte> m_out;

  void tag(uint32_t number, uint32_t wireType) {
    writeVarint((static_cast<uint64_t>(number) << 3U) | wireType);
  }
  void writeVarint(uint64_t value) {
    while (value >= 0x80U) {
      m_out.push_back(std::byte{static_cast<uint8_t>(value | 0x80U)});
      value >>= 7U;
    }
    m_out.push_back(std::byte{static_cast<uint8_t>(value)});
  }
  void raw(const void* p, size_t size) {
    const std::span<const std::byte> bytes(static_cast<const std::byte*>(p),
                                           size);
    m_out.insert(m_out.end(), bytes.begin(), bytes.end());
  }
};

template <typename T>
void append(std::vector<std::byte>& out, T value) {
  std::array<std::byte, sizeof(T)> buffer{};
  std::memcpy(buffer.data(), &value, sizeof(T));
  out.insert(out.end(), buffer.begin(), buffer.end());
}

void expectColor(const Color& c, float r, float g, float b, float a) {
  EXPECT_FLOAT_EQ(c.r, r);
  EXPECT_FLOAT_EQ(c.g, g);
  EXPECT_FLOAT_EQ(c.b, b);
  EXPECT_FLOAT_EQ(c.a, a);
}

void packedField(ProtoWriter& cloud, std::string_view name, uint32_t offset,
                 NumericType type) {
  ProtoWriter f;
  f.string(1, name);
  f.fixed32(2, offset);
  f.varint(3, static_cast<uint64_t>(type));
  cloud.message(5, f);
}

}  // namespace

TEST(FoxgloveDecoderTest, PointCloudRoundTripsFloat32Fields) {
  ProtoWriter w;
  w.timestamp(1, 1'700'000'000'123'456'789ULL);
  w.string(2, "LIDAR_TOP");
  w.pose(3, 1.0, 2.0, 3.0);
  w.fixed32(4, 16);
  packedField(w, "x", 0, NumericType::kFloat32);
  packedField(w, "y", 4, NumericType::kFloat32);
  packedField(w, "z", 8, NumericType::kFloat32);
  packedField(w, "intensity", 12, NumericType::kFloat32);
  std::vector<std::byte> data;
  for (int i = 0; i < 3; ++i) {
    append(data, static_cast<float>(i));
    append(data, static_cast<float>(i) * 10.0F);
    append(data, static_cast<float>(i) * -1.0F);
    append(data, 100.0F + static_cast<float>(i));
  }
  w.bytes(6, data);

  const PointCloud cloud = decodePointCloud(w.data());
  EXPECT_EQ(cloud.timestamp, 1'700'000'000'123'456'789ULL);
  EXPECT_EQ(cloud.frameId, "LIDAR_TOP");
  EXPECT_DOUBLE_EQ(cloud.pose.position.y, 2.0);
  EXPECT_DOUBLE_EQ(cloud.pose.orientation.w, 1.0);
  EXPECT_EQ(cloud.pointStride, 16U);
  ASSERT_EQ(cloud.fields.size(), 4U);
  EXPECT_EQ(cloud.fields[3].name, "intensity");
  EXPECT_EQ(cloud.fields[3].offset, 12U);
  EXPECT_EQ(cloud.fields[3].type, NumericType::kFloat32);
  EXPECT_EQ(cloud.pointCount(), 3U);

  const auto points = cloud.positionsAndIntensity();
  ASSERT_EQ(points.size(), 3U);
  EXPECT_FLOAT_EQ(points[2].x, 2.0F);
  EXPECT_FLOAT_EQ(points[2].y, 20.0F);
  EXPECT_FLOAT_EQ(points[2].z, -2.0F);
  EXPECT_FLOAT_EQ(points[2].w, 102.0F);
}

TEST(FoxgloveDecoderTest, PointCloudResolvesMixedNumericTypes) {
  ProtoWriter w;
  w.fixed32(4, 12);
  packedField(w, "intensity", 11, NumericType::kUint8);
  packedField(w, "x", 0, NumericType::kFloat64);
  packedField(w, "y", 8, NumericType::kInt16);
  packedField(w, "z", 10, NumericType::kInt8);
  std::vector<std::byte> data;
  append(data, 1.25);
  append(data, static_cast<int16_t>(-300));
  append(data, static_cast<int8_t>(-7));
  append(data, static_cast<uint8_t>(200));
  append(data, -2.5);
  append(data, static_cast<int16_t>(12));
  append(data, static_cast<int8_t>(3));
  append(data, static_cast<uint8_t>(0));
  w.bytes(6, data);

  const auto points = decodePointCloud(w.data()).positionsAndIntensity();
  ASSERT_EQ(points.size(), 2U);
  EXPECT_FLOAT_EQ(points[0].x, 1.25F);
  EXPECT_FLOAT_EQ(points[0].y, -300.0F);
  EXPECT_FLOAT_EQ(points[0].z, -7.0F);
  EXPECT_FLOAT_EQ(points[0].w, 200.0F);
  EXPECT_FLOAT_EQ(points[1].x, -2.5F);
  EXPECT_FLOAT_EQ(points[1].y, 12.0F);
  EXPECT_FLOAT_EQ(points[1].z, 3.0F);
  EXPECT_FLOAT_EQ(points[1].w, 0.0F);
}

TEST(FoxgloveDecoderTest, PointCloudMissingFieldsReadAsZero) {
  ProtoWriter w;
  w.fixed32(4, 8);
  packedField(w, "x", 0, NumericType::kFloat32);
  packedField(w, "y", 4, NumericType::kFloat32);
  std::vector<std::byte> data;
  append(data, 3.0F);
  append(data, 4.0F);
  w.bytes(6, data);
  const auto points = decodePointCloud(w.data()).positionsAndIntensity();
  ASSERT_EQ(points.size(), 1U);
  EXPECT_FLOAT_EQ(points[0].z, 0.0F);
  EXPECT_FLOAT_EQ(points[0].w, 0.0F);
}

TEST(FoxgloveDecoderTest, PointCloudRejectsFieldsOutsideTheStride) {
  ProtoWriter w;
  w.fixed32(4, 8);
  packedField(w, "x", 6, NumericType::kFloat32);
  std::vector<std::byte> data(8);
  w.bytes(6, data);
  EXPECT_THROW((void)decodePointCloud(w.data()).positionsAndIntensity(),
               std::runtime_error);

  ProtoWriter unknown;
  unknown.fixed32(4, 8);
  packedField(unknown, "x", 0, NumericType::kUnknown);
  unknown.bytes(6, data);
  EXPECT_THROW((void)decodePointCloud(unknown.data()).positionsAndIntensity(),
               std::runtime_error);
}

TEST(FoxgloveDecoderTest, EmptyPointCloudHasNoPoints) {
  const PointCloud cloud = decodePointCloud({});
  EXPECT_EQ(cloud.pointCount(), 0U);
  EXPECT_TRUE(cloud.positionsAndIntensity().empty());
}

TEST(FoxgloveDecoderTest, CompressedImageRoundTrips) {
  ProtoWriter w;
  w.timestamp(1, 5 * kNanosPerSecond + 7);
  const std::vector<std::byte> jpeg{std::byte{0xFF}, std::byte{0xD8},
                                    std::byte{0x00}};
  w.bytes(2, jpeg);
  w.string(3, "jpeg");
  w.string(4, "CAM_FRONT");
  const CompressedImage image = decodeCompressedImage(w.data());
  EXPECT_EQ(image.timestamp, 5 * kNanosPerSecond + 7);
  EXPECT_EQ(image.data, jpeg);
  EXPECT_EQ(image.format, "jpeg");
  EXPECT_EQ(image.frameId, "CAM_FRONT");
}

TEST(FoxgloveDecoderTest, FrameTransformRoundTrips) {
  ProtoWriter w;
  w.timestamp(1, 42);
  w.string(2, "map");
  w.string(3, "base_link");
  w.vec3(4, 1.0, -2.0, 0.5);
  w.quat(5, 0.0, 0.0, 0.7071, 0.7071);
  const FrameTransform tf = decodeFrameTransform(w.data());
  EXPECT_EQ(tf.timestamp, 42U);
  EXPECT_EQ(tf.parentFrameId, "map");
  EXPECT_EQ(tf.childFrameId, "base_link");
  EXPECT_DOUBLE_EQ(tf.translation.y, -2.0);
  EXPECT_DOUBLE_EQ(tf.rotation.z, 0.7071);
  EXPECT_DOUBLE_EQ(tf.rotation.w, 0.7071);
}

TEST(FoxgloveDecoderTest, UnknownFieldsAreIgnored) {
  ProtoWriter w;
  w.string(2, "map");
  w.varint(200, 7);
  w.string(201, "future");
  w.doubleField(202, 1.0);
  w.fixed32(203, 3);
  w.string(3, "child");
  const FrameTransform tf = decodeFrameTransform(w.data());
  EXPECT_EQ(tf.parentFrameId, "map");
  EXPECT_EQ(tf.childFrameId, "child");
}

TEST(FoxgloveDecoderTest, SceneUpdateRoundTripsEveryPrimitive) {
  ProtoWriter cube;
  cube.pose(1, 1.0, 2.0, 3.0);
  cube.vec3(2, 4.0, 2.0, 1.5);
  cube.color(3, 1.0, 0.0, 0.0, 0.5);

  ProtoWriter sphere;
  sphere.pose(1, 0.0, 0.0, 1.0);
  sphere.vec3(2, 0.5, 0.5, 0.5);
  sphere.color(3, 0.0, 1.0, 0.0, 1.0);

  ProtoWriter line;
  line.varint(1, static_cast<uint64_t>(LineType::kLineList));
  line.pose(2, 0.0, 0.0, 0.0);
  line.doubleField(3, 0.2);
  line.boolean(4, true);
  line.vec3(5, 0.0, 0.0, 0.0);
  line.vec3(5, 1.0, 0.0, 0.0);
  line.vec3(5, 1.0, 1.0, 0.0);
  line.color(6, 0.2, 0.8, 0.2, 1.0);
  line.color(7, 1.0, 0.0, 0.0, 1.0);
  line.color(7, 0.0, 1.0, 0.0, 1.0);
  line.color(7, 0.0, 0.0, 1.0, 1.0);
  const std::vector<uint32_t> indices{0, 1, 1, 2};
  line.packedFixed32(8, indices);

  ProtoWriter arrow;
  arrow.pose(1, 0.0, 0.0, 0.0);
  arrow.doubleField(2, 1.0);
  arrow.doubleField(3, 0.1);
  arrow.doubleField(4, 0.3);
  arrow.doubleField(5, 0.2);
  arrow.color(6, 1.0, 1.0, 0.0, 1.0);

  ProtoWriter text;
  text.pose(1, 0.0, 0.0, 2.0);
  text.boolean(2, true);
  text.doubleField(3, 0.5);
  text.boolean(4, false);
  text.color(5, 1.0, 1.0, 1.0, 1.0);
  text.string(6, "car");

  ProtoWriter metadata;
  metadata.string(1, "category");
  metadata.string(2, "vehicle.car");

  ProtoWriter entity;
  entity.timestamp(1, 10 * kNanosPerSecond);
  entity.string(2, "map");
  entity.string(3, "obj-1");
  {
    ProtoWriter lifetime;
    lifetime.varint(1, 0);
    lifetime.varint(2, 500'000'000);
    entity.message(4, lifetime);
  }
  entity.boolean(5, true);
  entity.message(6, metadata);
  entity.message(7, arrow);
  entity.message(8, cube);
  entity.message(8, cube);
  entity.message(9, sphere);
  entity.message(10, sphere);  // cylinder: counted only
  entity.message(11, line);
  entity.message(12, line);  // triangles: counted only
  entity.message(13, text);
  entity.message(14, text);  // model: counted only

  ProtoWriter deletion;
  deletion.timestamp(1, 9 * kNanosPerSecond);
  deletion.varint(2, static_cast<uint64_t>(DeletionType::kAll));
  deletion.string(3, "");

  ProtoWriter update;
  update.message(1, deletion);
  update.message(2, entity);

  const SceneUpdate decoded = decodeSceneUpdate(update.data());
  ASSERT_EQ(decoded.deletions.size(), 1U);
  EXPECT_EQ(decoded.deletions[0].timestamp, 9 * kNanosPerSecond);
  EXPECT_EQ(decoded.deletions[0].type, DeletionType::kAll);
  ASSERT_EQ(decoded.entities.size(), 1U);
  const SceneEntity& e = decoded.entities[0];
  EXPECT_EQ(e.timestamp, 10 * kNanosPerSecond);
  EXPECT_EQ(e.frameId, "map");
  EXPECT_EQ(e.id, "obj-1");
  EXPECT_EQ(e.lifetime, 500'000'000U);
  EXPECT_TRUE(e.frameLocked);
  ASSERT_EQ(e.arrows.size(), 1U);
  EXPECT_DOUBLE_EQ(e.arrows[0].headDiameter, 0.2);
  expectColor(e.arrows[0].color, 1.0F, 1.0F, 0.0F, 1.0F);
  ASSERT_EQ(e.cubes.size(), 2U);
  EXPECT_DOUBLE_EQ(e.cubes[1].pose.position.z, 3.0);
  EXPECT_DOUBLE_EQ(e.cubes[1].size.x, 4.0);
  expectColor(e.cubes[1].color, 1.0F, 0.0F, 0.0F, 0.5F);
  ASSERT_EQ(e.spheres.size(), 1U);
  EXPECT_DOUBLE_EQ(e.spheres[0].size.y, 0.5);
  ASSERT_EQ(e.lines.size(), 1U);
  EXPECT_EQ(e.lines[0].type, LineType::kLineList);
  EXPECT_DOUBLE_EQ(e.lines[0].thickness, 0.2);
  EXPECT_TRUE(e.lines[0].scaleInvariant);
  ASSERT_EQ(e.lines[0].points.size(), 3U);
  EXPECT_DOUBLE_EQ(e.lines[0].points[2].y, 1.0);
  expectColor(e.lines[0].color, 0.2F, 0.8F, 0.2F, 1.0F);
  ASSERT_EQ(e.lines[0].colors.size(), 3U);
  expectColor(e.lines[0].colors[2], 0.0F, 0.0F, 1.0F, 1.0F);
  EXPECT_EQ(e.lines[0].indices, indices);
  ASSERT_EQ(e.texts.size(), 1U);
  EXPECT_EQ(e.texts[0].text, "car");
  EXPECT_TRUE(e.texts[0].billboard);
  EXPECT_DOUBLE_EQ(e.texts[0].fontSize, 0.5);
  EXPECT_EQ(e.cylinderCount, 1U);
  EXPECT_EQ(e.triangleCount, 1U);
  EXPECT_EQ(e.modelCount, 1U);
}

TEST(FoxgloveDecoderTest, ImageAnnotationsRoundTrip) {
  ProtoWriter circle;
  circle.timestamp(1, 1);
  circle.vec2(2, 10.0, 20.0);
  circle.doubleField(3, 5.0);
  circle.doubleField(4, 1.5);
  circle.color(5, 0.0, 0.0, 1.0, 0.3);
  circle.color(6, 1.0, 1.0, 1.0, 1.0);

  ProtoWriter points;
  points.timestamp(1, 2);
  points.varint(2, static_cast<uint64_t>(PointsType::kLineLoop));
  points.vec2(3, 0.0, 0.0);
  points.vec2(3, 100.0, 0.0);
  points.vec2(3, 100.0, 50.0);
  points.vec2(3, 0.0, 50.0);
  points.color(4, 0.0, 1.0, 0.0, 1.0);
  points.color(5, 1.0, 0.0, 0.0, 1.0);
  points.color(6, 0.0, 0.0, 0.0, 0.0);
  points.doubleField(7, 2.0);

  ProtoWriter text;
  text.timestamp(1, 3);
  text.vec2(2, 5.0, 6.0);
  text.string(3, "pedestrian");
  text.doubleField(4, 12.0);
  text.color(5, 1.0, 1.0, 1.0, 1.0);
  text.color(6, 0.0, 0.0, 0.0, 0.5);

  ProtoWriter w;
  w.message(1, circle);
  w.message(2, points);
  w.message(3, text);
  w.timestamp(5, 7 * kNanosPerSecond);

  const ImageAnnotations a = decodeImageAnnotations(w.data());
  EXPECT_EQ(a.timestamp, 7 * kNanosPerSecond);
  ASSERT_EQ(a.circles.size(), 1U);
  EXPECT_DOUBLE_EQ(a.circles[0].position.y, 20.0);
  EXPECT_DOUBLE_EQ(a.circles[0].diameter, 5.0);
  EXPECT_DOUBLE_EQ(a.circles[0].thickness, 1.5);
  expectColor(a.circles[0].fillColor, 0.0F, 0.0F, 1.0F, 0.3F);
  ASSERT_EQ(a.points.size(), 1U);
  EXPECT_EQ(a.points[0].type, PointsType::kLineLoop);
  ASSERT_EQ(a.points[0].points.size(), 4U);
  EXPECT_DOUBLE_EQ(a.points[0].points[2].x, 100.0);
  EXPECT_DOUBLE_EQ(a.points[0].points[2].y, 50.0);
  expectColor(a.points[0].outlineColor, 0.0F, 1.0F, 0.0F, 1.0F);
  ASSERT_EQ(a.points[0].outlineColors.size(), 1U);
  EXPECT_DOUBLE_EQ(a.points[0].thickness, 2.0);
  ASSERT_EQ(a.texts.size(), 1U);
  EXPECT_EQ(a.texts[0].text, "pedestrian");
  EXPECT_DOUBLE_EQ(a.texts[0].fontSize, 12.0);
  EXPECT_DOUBLE_EQ(a.texts[0].position.x, 5.0);
  expectColor(a.texts[0].backgroundColor, 0.0F, 0.0F, 0.0F, 0.5F);
}

TEST(FoxgloveDecoderTest, CameraCalibrationRoundTrips) {
  ProtoWriter w;
  w.timestamp(1, 3);
  w.fixed32(2, 1600);
  w.fixed32(3, 900);
  w.string(4, "plumb_bob");
  const std::vector<double> d{0.1, 0.2, 0.3, 0.4, 0.5};
  w.packedDoubles(5, d);
  const std::vector<double> k{1266.0, 0.0, 816.0, 0.0, 1266.0,
                              491.0,  0.0, 0.0,   1.0};
  w.packedDoubles(6, k);
  const std::vector<double> r{1, 0, 0, 0, 1, 0, 0, 0, 1};
  w.packedDoubles(7, r);
  const std::vector<double> p{1266.0, 0.0, 816.0, 0.0, 0.0, 1266.0,
                              491.0,  0.0, 0.0,   0.0, 1.0, 0.0};
  w.packedDoubles(8, p);
  w.string(9, "CAM_FRONT");

  const CameraCalibration c = decodeCameraCalibration(w.data());
  EXPECT_EQ(c.timestamp, 3U);
  EXPECT_EQ(c.width, 1600U);
  EXPECT_EQ(c.height, 900U);
  EXPECT_EQ(c.distortionModel, "plumb_bob");
  EXPECT_EQ(c.frameId, "CAM_FRONT");
  EXPECT_EQ(c.D, d);
  EXPECT_DOUBLE_EQ(c.K[0], 1266.0);
  EXPECT_DOUBLE_EQ(c.K[5], 491.0);
  EXPECT_DOUBLE_EQ(c.K[8], 1.0);
  EXPECT_DOUBLE_EQ(c.R[4], 1.0);
  EXPECT_DOUBLE_EQ(c.P[6], 491.0);
  EXPECT_DOUBLE_EQ(c.P[10], 1.0);
}

TEST(FoxgloveDecoderTest, MalformedInputThrowsWithTheSchemaName) {
  // A length-delimited field 6 claiming 100 bytes with none available.
  const std::vector<std::byte> truncated{std::byte{0x32}, std::byte{100}};
  try {
    (void)decodePointCloud(truncated);
    FAIL() << "expected an exception";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("foxglove.PointCloud"),
              std::string::npos);
  }
  EXPECT_THROW((void)decodeCompressedImage(truncated), std::runtime_error);
  EXPECT_THROW((void)decodeFrameTransform(truncated), std::runtime_error);
  EXPECT_THROW((void)decodeSceneUpdate(truncated), std::runtime_error);
  EXPECT_THROW((void)decodeImageAnnotations(truncated), std::runtime_error);
  EXPECT_THROW((void)decodeCameraCalibration(truncated), std::runtime_error);
}

TEST(FoxgloveDecoderTest, WrongWireTypeForANestedMessageThrows) {
  // FrameTransform.translation (field 4) written as a varint.
  ProtoWriter w;
  w.varint(4, 1);
  EXPECT_THROW((void)decodeFrameTransform(w.data()), std::runtime_error);
  // PointCloud.point_stride (field 4) written as a varint instead of fixed32.
  ProtoWriter cloud;
  cloud.varint(4, 16);
  EXPECT_THROW((void)decodePointCloud(cloud.data()), std::runtime_error);
}

}  // namespace camelot

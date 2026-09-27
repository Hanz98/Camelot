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

#include "Camelot/src/data/FoxgloveDecoder.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Camelot/src/data/ProtoReader.h"

namespace camelot {

namespace {

constexpr uint64_t kNanosPerSecond = 1'000'000'000ULL;

using Bytes = std::span<const std::byte>;

// Runs `onField` for every field of `message`; the callback ignores the
// numbers it does not know, which is how unknown fields are skipped.
template <typename OnField>
void forEachField(Bytes message, const OnField& onField) {
  ProtoReader reader(message);
  ProtoField field;
  while (reader.next(field)) {
    onField(field);
  }
}

// Decodes a whole message, attaching the schema name to any error.
template <typename T, typename Decode>
T decodeMessage(const char* schema, Bytes message, const Decode& decode) {
  try {
    return decode(message);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(std::string(schema) + ": " + error.what());
  }
}

// google.protobuf.Timestamp and Duration share the layout
// {int64 seconds = 1; int32 nanos = 2}.
uint64_t decodeSecondsNanos(Bytes message) {
  int64_t seconds = 0;
  int64_t nanos = 0;
  forEachField(message, [&](const ProtoField& field) {
    if (field.number == 1) {
      seconds = ProtoReader::toInt(field);
    } else if (field.number == 2) {
      nanos = ProtoReader::toInt(field);
    }
  });
  if (seconds < 0 || nanos < 0) {
    throw std::runtime_error("negative timestamp");
  }
  return static_cast<uint64_t>(seconds) * kNanosPerSecond +
         static_cast<uint64_t>(nanos);
}

Time decodeTimestamp(Bytes message) { return decodeSecondsNanos(message); }

Duration decodeDuration(Bytes message) { return decodeSecondsNanos(message); }

Vec2 decodeVec2(Bytes message) {
  Vec2 v;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        v.x = ProtoReader::toDouble(field);
        break;
      case 2:
        v.y = ProtoReader::toDouble(field);
        break;
      default:
        break;
    }
  });
  return v;
}

Vec3 decodeVec3(Bytes message) {
  Vec3 v;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        v.x = ProtoReader::toDouble(field);
        break;
      case 2:
        v.y = ProtoReader::toDouble(field);
        break;
      case 3:
        v.z = ProtoReader::toDouble(field);
        break;
      default:
        break;
    }
  });
  return v;
}

Quat decodeQuat(Bytes message) {
  Quat q;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        q.x = ProtoReader::toDouble(field);
        break;
      case 2:
        q.y = ProtoReader::toDouble(field);
        break;
      case 3:
        q.z = ProtoReader::toDouble(field);
        break;
      case 4:
        q.w = ProtoReader::toDouble(field);
        break;
      default:
        break;
    }
  });
  return q;
}

Pose decodePose(Bytes message) {
  Pose pose;
  forEachField(message, [&](const ProtoField& field) {
    if (field.number == 1) {
      pose.position = decodeVec3(ProtoReader::toBytes(field));
    } else if (field.number == 2) {
      pose.orientation = decodeQuat(ProtoReader::toBytes(field));
    }
  });
  return pose;
}

Color decodeColor(Bytes message) {
  Color c;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        c.r = static_cast<float>(ProtoReader::toDouble(field));
        break;
      case 2:
        c.g = static_cast<float>(ProtoReader::toDouble(field));
        break;
      case 3:
        c.b = static_cast<float>(ProtoReader::toDouble(field));
        break;
      case 4:
        c.a = static_cast<float>(ProtoReader::toDouble(field));
        break;
      default:
        break;
    }
  });
  return c;
}

std::vector<std::byte> copyBytes(const ProtoField& field) {
  const Bytes bytes = ProtoReader::toBytes(field);
  return {bytes.begin(), bytes.end()};
}

std::string copyString(const ProtoField& field) {
  return std::string(ProtoReader::toString(field));
}

PackedField decodePackedField(Bytes message) {
  PackedField packed;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        packed.name = copyString(field);
        break;
      case 2:
        packed.offset = ProtoReader::toFixed32(field);
        break;
      case 3:
        packed.type = static_cast<NumericType>(ProtoReader::toInt(field));
        break;
      default:
        break;
    }
  });
  return packed;
}

CubePrimitive decodeCube(Bytes message) {
  CubePrimitive cube;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        cube.pose = decodePose(ProtoReader::toBytes(field));
        break;
      case 2:
        cube.size = decodeVec3(ProtoReader::toBytes(field));
        break;
      case 3:
        cube.color = decodeColor(ProtoReader::toBytes(field));
        break;
      default:
        break;
    }
  });
  return cube;
}

SpherePrimitive decodeSphere(Bytes message) {
  SpherePrimitive sphere;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        sphere.pose = decodePose(ProtoReader::toBytes(field));
        break;
      case 2:
        sphere.size = decodeVec3(ProtoReader::toBytes(field));
        break;
      case 3:
        sphere.color = decodeColor(ProtoReader::toBytes(field));
        break;
      default:
        break;
    }
  });
  return sphere;
}

LinePrimitive decodeLine(Bytes message) {
  LinePrimitive line;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        line.type = static_cast<LineType>(ProtoReader::toInt(field));
        break;
      case 2:
        line.pose = decodePose(ProtoReader::toBytes(field));
        break;
      case 3:
        line.thickness = ProtoReader::toDouble(field);
        break;
      case 4:
        line.scaleInvariant = ProtoReader::toBool(field);
        break;
      case 5:
        line.points.push_back(decodeVec3(ProtoReader::toBytes(field)));
        break;
      case 6:
        line.color = decodeColor(ProtoReader::toBytes(field));
        break;
      case 7:
        line.colors.push_back(decodeColor(ProtoReader::toBytes(field)));
        break;
      case 8:
        ProtoReader::packedUint32(field, line.indices);
        break;
      default:
        break;
    }
  });
  return line;
}

ArrowPrimitive decodeArrow(Bytes message) {
  ArrowPrimitive arrow;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        arrow.pose = decodePose(ProtoReader::toBytes(field));
        break;
      case 2:
        arrow.shaftLength = ProtoReader::toDouble(field);
        break;
      case 3:
        arrow.shaftDiameter = ProtoReader::toDouble(field);
        break;
      case 4:
        arrow.headLength = ProtoReader::toDouble(field);
        break;
      case 5:
        arrow.headDiameter = ProtoReader::toDouble(field);
        break;
      case 6:
        arrow.color = decodeColor(ProtoReader::toBytes(field));
        break;
      default:
        break;
    }
  });
  return arrow;
}

TextPrimitive decodeText(Bytes message) {
  TextPrimitive text;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        text.pose = decodePose(ProtoReader::toBytes(field));
        break;
      case 2:
        text.billboard = ProtoReader::toBool(field);
        break;
      case 3:
        text.fontSize = ProtoReader::toDouble(field);
        break;
      case 4:
        text.scaleInvariant = ProtoReader::toBool(field);
        break;
      case 5:
        text.color = decodeColor(ProtoReader::toBytes(field));
        break;
      case 6:
        text.text = copyString(field);
        break;
      default:
        break;
    }
  });
  return text;
}

// The header fields of SceneEntity (1..6); primitives are handled by
// decodeEntity.
void decodeEntityHeader(const ProtoField& field, SceneEntity& entity) {
  switch (field.number) {
    case 1:
      entity.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
      break;
    case 2:
      entity.frameId = copyString(field);
      break;
    case 3:
      entity.id = copyString(field);
      break;
    case 4:
      entity.lifetime = decodeDuration(ProtoReader::toBytes(field));
      break;
    case 5:
      entity.frameLocked = ProtoReader::toBool(field);
      break;
    default:
      break;  // 6: metadata, not needed.
  }
}

SceneEntity decodeEntity(Bytes message) {
  SceneEntity entity;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 7:
        entity.arrows.push_back(decodeArrow(ProtoReader::toBytes(field)));
        break;
      case 8:
        entity.cubes.push_back(decodeCube(ProtoReader::toBytes(field)));
        break;
      case 9:
        entity.spheres.push_back(decodeSphere(ProtoReader::toBytes(field)));
        break;
      case 10:
        ++entity.cylinderCount;
        break;
      case 11:
        entity.lines.push_back(decodeLine(ProtoReader::toBytes(field)));
        break;
      case 12:
        ++entity.triangleCount;
        break;
      case 13:
        entity.texts.push_back(decodeText(ProtoReader::toBytes(field)));
        break;
      case 14:
        ++entity.modelCount;
        break;
      default:
        decodeEntityHeader(field, entity);
        break;
    }
  });
  return entity;
}

SceneEntityDeletion decodeDeletion(Bytes message) {
  SceneEntityDeletion deletion;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        deletion.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
        break;
      case 2:
        deletion.type = static_cast<DeletionType>(ProtoReader::toInt(field));
        break;
      case 3:
        deletion.id = copyString(field);
        break;
      default:
        break;
    }
  });
  return deletion;
}

CircleAnnotation decodeCircle(Bytes message) {
  CircleAnnotation circle;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        circle.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
        break;
      case 2:
        circle.position = decodeVec2(ProtoReader::toBytes(field));
        break;
      case 3:
        circle.diameter = ProtoReader::toDouble(field);
        break;
      case 4:
        circle.thickness = ProtoReader::toDouble(field);
        break;
      case 5:
        circle.fillColor = decodeColor(ProtoReader::toBytes(field));
        break;
      case 6:
        circle.outlineColor = decodeColor(ProtoReader::toBytes(field));
        break;
      default:
        break;
    }
  });
  return circle;
}

PointsAnnotation decodePoints(Bytes message) {
  PointsAnnotation points;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        points.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
        break;
      case 2:
        points.type = static_cast<PointsType>(ProtoReader::toInt(field));
        break;
      case 3:
        points.points.push_back(decodeVec2(ProtoReader::toBytes(field)));
        break;
      case 4:
        points.outlineColor = decodeColor(ProtoReader::toBytes(field));
        break;
      case 5:
        points.outlineColors.push_back(
            decodeColor(ProtoReader::toBytes(field)));
        break;
      case 6:
        points.fillColor = decodeColor(ProtoReader::toBytes(field));
        break;
      case 7:
        points.thickness = ProtoReader::toDouble(field);
        break;
      default:
        break;
    }
  });
  return points;
}

TextAnnotation decodeTextAnnotation(Bytes message) {
  TextAnnotation text;
  forEachField(message, [&](const ProtoField& field) {
    switch (field.number) {
      case 1:
        text.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
        break;
      case 2:
        text.position = decodeVec2(ProtoReader::toBytes(field));
        break;
      case 3:
        text.text = copyString(field);
        break;
      case 4:
        text.fontSize = ProtoReader::toDouble(field);
        break;
      case 5:
        text.textColor = decodeColor(ProtoReader::toBytes(field));
        break;
      case 6:
        text.backgroundColor = decodeColor(ProtoReader::toBytes(field));
        break;
      default:
        break;
    }
  });
  return text;
}

// Copies a `repeated double` into a fixed-size array, ignoring extra
// elements; the schema documents the expected lengths.
template <size_t N>
void fillArray(const ProtoField& field, std::array<double, N>& out) {
  std::vector<double> values;
  ProtoReader::packedDoubles(field, values);
  const size_t count = std::min(values.size(), N);
  std::copy_n(values.begin(), count, out.begin());
}

}  // namespace

PointCloud decodePointCloud(Bytes message) {
  return decodeMessage<PointCloud>("foxglove.PointCloud", message, [](Bytes m) {
    PointCloud cloud;
    forEachField(m, [&](const ProtoField& field) {
      switch (field.number) {
        case 1:
          cloud.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
          break;
        case 2:
          cloud.frameId = copyString(field);
          break;
        case 3:
          cloud.pose = decodePose(ProtoReader::toBytes(field));
          break;
        case 4:
          cloud.pointStride = ProtoReader::toFixed32(field);
          break;
        case 5:
          cloud.fields.push_back(
              decodePackedField(ProtoReader::toBytes(field)));
          break;
        case 6:
          cloud.data = copyBytes(field);
          break;
        default:
          break;
      }
    });
    return cloud;
  });
}

CompressedImage decodeCompressedImage(Bytes message) {
  return decodeMessage<CompressedImage>(
      "foxglove.CompressedImage", message, [](Bytes m) {
        CompressedImage image;
        forEachField(m, [&](const ProtoField& field) {
          switch (field.number) {
            case 1:
              image.timestamp = decodeTimestamp(ProtoReader::toBytes(field));
              break;
            case 2:
              image.data = copyBytes(field);
              break;
            case 3:
              image.format = copyString(field);
              break;
            case 4:
              image.frameId = copyString(field);
              break;
            default:
              break;
          }
        });
        return image;
      });
}

FrameTransform decodeFrameTransform(Bytes message) {
  return decodeMessage<FrameTransform>(
      "foxglove.FrameTransform", message, [](Bytes m) {
        FrameTransform transform;
        forEachField(m, [&](const ProtoField& field) {
          switch (field.number) {
            case 1:
              transform.timestamp =
                  decodeTimestamp(ProtoReader::toBytes(field));
              break;
            case 2:
              transform.parentFrameId = copyString(field);
              break;
            case 3:
              transform.childFrameId = copyString(field);
              break;
            case 4:
              transform.translation = decodeVec3(ProtoReader::toBytes(field));
              break;
            case 5:
              transform.rotation = decodeQuat(ProtoReader::toBytes(field));
              break;
            default:
              break;
          }
        });
        return transform;
      });
}

SceneUpdate decodeSceneUpdate(Bytes message) {
  return decodeMessage<SceneUpdate>(
      "foxglove.SceneUpdate", message, [](Bytes m) {
        SceneUpdate update;
        forEachField(m, [&](const ProtoField& field) {
          if (field.number == 1) {
            update.deletions.push_back(
                decodeDeletion(ProtoReader::toBytes(field)));
          } else if (field.number == 2) {
            update.entities.push_back(
                decodeEntity(ProtoReader::toBytes(field)));
          }
        });
        return update;
      });
}

ImageAnnotations decodeImageAnnotations(Bytes message) {
  return decodeMessage<ImageAnnotations>(
      "foxglove.ImageAnnotations", message, [](Bytes m) {
        ImageAnnotations annotations;
        forEachField(m, [&](const ProtoField& field) {
          switch (field.number) {
            case 1:
              annotations.circles.push_back(
                  decodeCircle(ProtoReader::toBytes(field)));
              break;
            case 2:
              annotations.points.push_back(
                  decodePoints(ProtoReader::toBytes(field)));
              break;
            case 3:
              annotations.texts.push_back(
                  decodeTextAnnotation(ProtoReader::toBytes(field)));
              break;
            case 5:
              annotations.timestamp =
                  decodeTimestamp(ProtoReader::toBytes(field));
              break;
            default:
              break;  // 4: metadata.
          }
        });
        return annotations;
      });
}

CameraCalibration decodeCameraCalibration(Bytes message) {
  return decodeMessage<CameraCalibration>(
      "foxglove.CameraCalibration", message, [](Bytes m) {
        CameraCalibration calibration;
        forEachField(m, [&](const ProtoField& field) {
          switch (field.number) {
            case 1:
              calibration.timestamp =
                  decodeTimestamp(ProtoReader::toBytes(field));
              break;
            case 2:
              calibration.width = ProtoReader::toFixed32(field);
              break;
            case 3:
              calibration.height = ProtoReader::toFixed32(field);
              break;
            case 4:
              calibration.distortionModel = copyString(field);
              break;
            case 5:
              ProtoReader::packedDoubles(field, calibration.D);
              break;
            case 6:
              fillArray(field, calibration.K);
              break;
            case 7:
              fillArray(field, calibration.R);
              break;
            case 8:
              fillArray(field, calibration.P);
              break;
            case 9:
              calibration.frameId = copyString(field);
              break;
            default:
              break;
          }
        });
        return calibration;
      });
}

}  // namespace camelot

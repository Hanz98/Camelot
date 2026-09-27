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

#include "Camelot/src/data/FoxgloveMessages.h"

#include <cstring>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace camelot {

namespace {

// Byte size of one element of the given type, 0 for kUnknown.
size_t elementSize(NumericType type) {
  switch (type) {
    case NumericType::kUint8:
    case NumericType::kInt8:
      return 1;
    case NumericType::kUint16:
    case NumericType::kInt16:
      return 2;
    case NumericType::kUint32:
    case NumericType::kInt32:
    case NumericType::kFloat32:
      return 4;
    case NumericType::kFloat64:
      return sizeof(double);
    case NumericType::kUnknown:
      return 0;
  }
  return 0;
}

template <typename T>
float readAs(const std::byte* at) {
  T value{};
  std::memcpy(&value, at, sizeof(T));
  return static_cast<float>(value);
}

float readNumber(const std::byte* at, NumericType type) {
  switch (type) {
    case NumericType::kUint8:
      return readAs<uint8_t>(at);
    case NumericType::kInt8:
      return readAs<int8_t>(at);
    case NumericType::kUint16:
      return readAs<uint16_t>(at);
    case NumericType::kInt16:
      return readAs<int16_t>(at);
    case NumericType::kUint32:
      return readAs<uint32_t>(at);
    case NumericType::kInt32:
      return readAs<int32_t>(at);
    case NumericType::kFloat32:
      return readAs<float>(at);
    case NumericType::kFloat64:
      return readAs<double>(at);
    case NumericType::kUnknown:
      return 0.0F;
  }
  return 0.0F;
}

// A resolved field: where to read it in every point, or absent.
struct Accessor {
  bool present{false};
  size_t offset{0};
  NumericType type{NumericType::kUnknown};
};

Accessor resolve(const PointCloud& cloud, std::string_view name) {
  for (const PackedField& field : cloud.fields) {
    if (field.name != name) {
      continue;
    }
    const size_t size = elementSize(field.type);
    if (size == 0) {
      throw std::runtime_error("PointCloud: field '" + field.name +
                               "' has an unknown numeric type");
    }
    if (field.offset + size > cloud.pointStride) {
      throw std::runtime_error("PointCloud: field '" + field.name +
                               "' lies outside the point stride");
    }
    return {.present = true, .offset = field.offset, .type = field.type};
  }
  return {};
}

float readField(std::span<const std::byte> point, const Accessor& accessor) {
  return accessor.present
             ? readNumber(point.subspan(accessor.offset).data(), accessor.type)
             : 0.0F;
}

}  // namespace

size_t PointCloud::pointCount() const {
  return pointStride == 0 ? 0 : data.size() / pointStride;
}

std::vector<glm::vec4> PointCloud::positionsAndIntensity() const {
  std::vector<glm::vec4> out;
  const size_t count = pointCount();
  if (count == 0) {
    return out;
  }
  const Accessor x = resolve(*this, "x");
  const Accessor y = resolve(*this, "y");
  const Accessor z = resolve(*this, "z");
  const Accessor intensity = resolve(*this, "intensity");
  out.reserve(count);
  const std::span<const std::byte> all(data);
  for (size_t i = 0; i < count; ++i) {
    const std::span<const std::byte> point =
        all.subspan(i * pointStride, pointStride);
    out.emplace_back(readField(point, x), readField(point, y),
                     readField(point, z), readField(point, intensity));
  }
  return out;
}

}  // namespace camelot

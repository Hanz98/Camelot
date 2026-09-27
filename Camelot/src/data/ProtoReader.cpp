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

#include "Camelot/src/data/ProtoReader.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace camelot {

namespace {

constexpr size_t kMaxVarintBytes = 10;
constexpr unsigned kVarintDataBits = 7;
constexpr uint8_t kVarintContinuation = 0x80;
constexpr uint8_t kVarintMask = 0x7F;
constexpr unsigned kTagTypeBits = 3;
constexpr uint32_t kTagTypeMask = 0x7;
constexpr size_t kFixed32Size = 4;
constexpr size_t kFixed64Size = 8;

void requireType(const ProtoField& field, WireType type, const char* what) {
  if (field.type != type) {
    throw std::runtime_error(std::string("protobuf: field ") +
                             std::to_string(field.number) +
                             " has the wrong wire type for " + what);
  }
}

template <typename T>
T readLittleEndian(std::span<const std::byte> bytes) {
  T value{};
  std::memcpy(&value, bytes.data(), sizeof(T));
  return value;
}

}  // namespace

ProtoReader::ProtoReader(std::span<const std::byte> message)
    : m_remaining(message) {}

uint64_t ProtoReader::readVarint(std::span<const std::byte>& bytes) {
  uint64_t value = 0;
  for (size_t i = 0; i < kMaxVarintBytes; ++i) {
    if (i >= bytes.size()) {
      throw std::runtime_error("protobuf: truncated varint");
    }
    const auto byte = static_cast<uint8_t>(bytes[i]);
    value |= static_cast<uint64_t>(byte & kVarintMask) << (kVarintDataBits * i);
    if ((byte & kVarintContinuation) == 0) {
      bytes = bytes.subspan(i + 1);
      return value;
    }
  }
  throw std::runtime_error("protobuf: varint longer than 10 bytes");
}

bool ProtoReader::next(ProtoField& field) {
  if (m_remaining.empty()) {
    return false;
  }
  const uint64_t tag = readVarint(m_remaining);
  field.number = static_cast<uint32_t>(tag >> kTagTypeBits);
  if (field.number == 0) {
    throw std::runtime_error("protobuf: field number 0");
  }
  field.varint = 0;
  field.bytes = {};
  switch (static_cast<uint32_t>(tag & kTagTypeMask)) {
    case static_cast<uint32_t>(WireType::kVarint):
      field.type = WireType::kVarint;
      field.varint = readVarint(m_remaining);
      return true;
    case static_cast<uint32_t>(WireType::kFixed64):
      field.type = WireType::kFixed64;
      readFixed(kFixed64Size, field);
      return true;
    case static_cast<uint32_t>(WireType::kLengthDelimited):
      field.type = WireType::kLengthDelimited;
      readLengthDelimited(field);
      return true;
    case static_cast<uint32_t>(WireType::kFixed32):
      field.type = WireType::kFixed32;
      readFixed(kFixed32Size, field);
      return true;
    default:
      throw std::runtime_error("protobuf: unsupported wire type " +
                               std::to_string(tag & kTagTypeMask) +
                               " in field " + std::to_string(field.number));
  }
}

void ProtoReader::readFixed(size_t size, ProtoField& field) {
  if (m_remaining.size() < size) {
    throw std::runtime_error("protobuf: truncated fixed-width field " +
                             std::to_string(field.number));
  }
  field.bytes = m_remaining.first(size);
  m_remaining = m_remaining.subspan(size);
}

void ProtoReader::readLengthDelimited(ProtoField& field) {
  const uint64_t length = readVarint(m_remaining);
  if (length > m_remaining.size()) {
    throw std::runtime_error("protobuf: truncated length-delimited field " +
                             std::to_string(field.number));
  }
  field.bytes = m_remaining.first(static_cast<size_t>(length));
  m_remaining = m_remaining.subspan(static_cast<size_t>(length));
}

double ProtoReader::toDouble(const ProtoField& field) {
  requireType(field, WireType::kFixed64, "double");
  return readLittleEndian<double>(field.bytes);
}

float ProtoReader::toFloat(const ProtoField& field) {
  requireType(field, WireType::kFixed32, "float");
  return readLittleEndian<float>(field.bytes);
}

uint32_t ProtoReader::toFixed32(const ProtoField& field) {
  requireType(field, WireType::kFixed32, "fixed32");
  return readLittleEndian<uint32_t>(field.bytes);
}

uint64_t ProtoReader::toFixed64(const ProtoField& field) {
  requireType(field, WireType::kFixed64, "fixed64");
  return readLittleEndian<uint64_t>(field.bytes);
}

int64_t ProtoReader::toSint(const ProtoField& field) {
  requireType(field, WireType::kVarint, "sint");
  const uint64_t n = field.varint;
  return static_cast<int64_t>(n >> 1U) ^ -static_cast<int64_t>(n & 1U);
}

int64_t ProtoReader::toInt(const ProtoField& field) {
  requireType(field, WireType::kVarint, "int");
  return static_cast<int64_t>(field.varint);
}

bool ProtoReader::toBool(const ProtoField& field) {
  requireType(field, WireType::kVarint, "bool");
  return field.varint != 0;
}

std::string_view ProtoReader::toString(const ProtoField& field) {
  requireType(field, WireType::kLengthDelimited, "string");
  // Viewing std::byte storage as characters is the intended use of the cast.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* chars = reinterpret_cast<const char*>(field.bytes.data());
  return {chars, field.bytes.size()};
}

std::span<const std::byte> ProtoReader::toBytes(const ProtoField& field) {
  requireType(field, WireType::kLengthDelimited, "bytes or message");
  return field.bytes;
}

void ProtoReader::packedDoubles(const ProtoField& field,
                                std::vector<double>& out) {
  if (field.type == WireType::kFixed64) {
    out.push_back(readLittleEndian<double>(field.bytes));
    return;
  }
  requireType(field, WireType::kLengthDelimited, "packed double");
  if (field.bytes.size() % kFixed64Size != 0) {
    throw std::runtime_error("protobuf: packed doubles of field " +
                             std::to_string(field.number) +
                             " are not a multiple of 8 bytes");
  }
  for (size_t i = 0; i < field.bytes.size(); i += kFixed64Size) {
    out.push_back(readLittleEndian<double>(field.bytes.subspan(i)));
  }
}

void ProtoReader::packedUint32(const ProtoField& field,
                               std::vector<uint32_t>& out) {
  if (field.type == WireType::kFixed32) {
    out.push_back(readLittleEndian<uint32_t>(field.bytes));
    return;
  }
  requireType(field, WireType::kLengthDelimited, "packed fixed32");
  if (field.bytes.size() % kFixed32Size != 0) {
    throw std::runtime_error("protobuf: packed fixed32 of field " +
                             std::to_string(field.number) +
                             " are not a multiple of 4 bytes");
  }
  for (size_t i = 0; i < field.bytes.size(); i += kFixed32Size) {
    out.push_back(readLittleEndian<uint32_t>(field.bytes.subspan(i)));
  }
}

void ProtoReader::packedVarints(const ProtoField& field,
                                std::vector<uint64_t>& out) {
  if (field.type == WireType::kVarint) {
    out.push_back(field.varint);
    return;
  }
  requireType(field, WireType::kLengthDelimited, "packed varints");
  std::span<const std::byte> rest = field.bytes;
  while (!rest.empty()) {
    out.push_back(readVarint(rest));
  }
}

}  // namespace camelot

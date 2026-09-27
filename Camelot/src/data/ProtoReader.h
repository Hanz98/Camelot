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

#ifndef CAMELOT_SRC_DATA_PROTOREADER_H_
#define CAMELOT_SRC_DATA_PROTOREADER_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace camelot {

// The protobuf wire types this reader understands. Groups (3 and 4) are
// deprecated and never appear in Foxglove messages; they are rejected.
enum class WireType : uint8_t {
  kVarint = 0,
  kFixed64 = 1,
  kLengthDelimited = 2,
  kFixed32 = 5,
};

// One field of a protobuf message as it appears on the wire. `varint` holds
// the value of a varint field; `bytes` spans the payload of a
// length-delimited field or the 4/8 raw bytes of a fixed-width field.
struct ProtoField {
  uint32_t number{0};
  WireType type{WireType::kVarint};
  uint64_t varint{0};
  std::span<const std::byte> bytes;
};

// Iterates the fields of one protobuf message without a schema. Nested
// messages are new readers over `ProtoField::bytes`. Unknown fields are the
// caller's business to skip (just do not act on them); malformed or truncated
// input throws std::runtime_error. The reader does not own the bytes.
class ProtoReader {
 public:
  explicit ProtoReader(std::span<const std::byte> message);

  // Reads the next field into `field`; returns false at the end of the
  // message.
  bool next(ProtoField& field);

  // Converters check the wire type and throw std::runtime_error on mismatch.
  [[nodiscard]] static double toDouble(const ProtoField& field);
  [[nodiscard]] static float toFloat(const ProtoField& field);
  [[nodiscard]] static uint32_t toFixed32(const ProtoField& field);
  [[nodiscard]] static uint64_t toFixed64(const ProtoField& field);
  // Zigzag-decoded sint32/sint64.
  [[nodiscard]] static int64_t toSint(const ProtoField& field);
  // Two's-complement int32/int64 (the encoding of plain `int32` fields).
  [[nodiscard]] static int64_t toInt(const ProtoField& field);
  [[nodiscard]] static bool toBool(const ProtoField& field);
  [[nodiscard]] static std::string_view toString(const ProtoField& field);
  // The payload of a length-delimited field (bytes or a nested message).
  [[nodiscard]] static std::span<const std::byte> toBytes(
      const ProtoField& field);

  // Appends the elements of a `repeated double` field. Accepts both the packed
  // encoding and a single unpacked fixed64 element.
  static void packedDoubles(const ProtoField& field, std::vector<double>& out);
  // Appends the elements of a `repeated fixed32` field (packed or a single
  // unpacked element).
  static void packedUint32(const ProtoField& field, std::vector<uint32_t>& out);
  // Appends the elements of a packed `repeated uint32/int32/enum` (varints).
  static void packedVarints(const ProtoField& field,
                            std::vector<uint64_t>& out);

  // Decodes one varint from the front of `bytes` and shrinks it accordingly;
  // throws on truncation or overlong encoding.
  static uint64_t readVarint(std::span<const std::byte>& bytes);

 private:
  std::span<const std::byte> m_remaining;

  void readFixed(size_t size, ProtoField& field);
  void readLengthDelimited(ProtoField& field);
};

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_PROTOREADER_H_

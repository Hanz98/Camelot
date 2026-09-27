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

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Camelot/src/data/ProtoReader.h"

namespace camelot {

namespace {

std::vector<std::byte> bytes(std::initializer_list<uint8_t> values) {
  std::vector<std::byte> out;
  for (const uint8_t v : values) {
    out.push_back(std::byte{v});
  }
  return out;
}

// Every field of a message. Owns the bytes, so the spans inside the fields
// stay valid for as long as the test holds the result.
class Fields {
 public:
  explicit Fields(std::vector<std::byte> message)
      : m_message(std::move(message)) {
    ProtoReader reader(m_message);
    ProtoField field;
    while (reader.next(field)) {
      m_fields.push_back(field);
    }
  }
  [[nodiscard]] const ProtoField& operator[](size_t i) const {
    return m_fields[i];
  }
  [[nodiscard]] size_t size() const { return m_fields.size(); }

 private:
  std::vector<std::byte> m_message;
  std::vector<ProtoField> m_fields;
};

Fields readAll(std::vector<std::byte> message) {
  return Fields(std::move(message));
}

}  // namespace

TEST(ProtoReaderTest, EmptyMessageHasNoFields) {
  const std::vector<std::byte> empty;
  ProtoReader reader(empty);
  ProtoField field;
  EXPECT_FALSE(reader.next(field));
}

TEST(ProtoReaderTest, ReadsVarints) {
  // field 1 = 300 (0xAC 0x02), field 2 = 1, field 3 = 0.
  const auto fields =
      readAll(bytes({0x08, 0xAC, 0x02, 0x10, 0x01, 0x18, 0x00}));
  ASSERT_EQ(fields.size(), 3U);
  EXPECT_EQ(fields[0].number, 1U);
  EXPECT_EQ(fields[0].type, WireType::kVarint);
  EXPECT_EQ(fields[0].varint, 300U);
  EXPECT_EQ(fields[1].number, 2U);
  EXPECT_TRUE(ProtoReader::toBool(fields[1]));
  EXPECT_EQ(fields[2].number, 3U);
  EXPECT_FALSE(ProtoReader::toBool(fields[2]));
  EXPECT_EQ(ProtoReader::toInt(fields[0]), 300);
}

TEST(ProtoReaderTest, ReadsTenByteVarintAndNegativeInt) {
  // -1 as int32/int64 is the ten-byte varint of UINT64_MAX.
  const auto fields = readAll(bytes(
      {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}));
  ASSERT_EQ(fields.size(), 1U);
  EXPECT_EQ(fields[0].varint, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(ProtoReader::toInt(fields[0]), -1);
}

TEST(ProtoReaderTest, DecodesZigzagSints) {
  // sint32 values 0, -1, 1, -2, 2 encode as varints 0, 1, 2, 3, 4.
  const auto fields =
      readAll(bytes({0x08, 0x00, 0x08, 0x01, 0x08, 0x02, 0x08, 0x03, 0x08, 0x04,
                     0x08, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F}));
  ASSERT_EQ(fields.size(), 6U);
  EXPECT_EQ(ProtoReader::toSint(fields[0]), 0);
  EXPECT_EQ(ProtoReader::toSint(fields[1]), -1);
  EXPECT_EQ(ProtoReader::toSint(fields[2]), 1);
  EXPECT_EQ(ProtoReader::toSint(fields[3]), -2);
  EXPECT_EQ(ProtoReader::toSint(fields[4]), 2);
  EXPECT_EQ(ProtoReader::toSint(fields[5]), 2147483647);
}

TEST(ProtoReaderTest, ReadsFixed32AndFloat) {
  // field 3, wire type 5: float 1.5 = 0x3FC00000 little-endian.
  const auto fields = readAll(bytes({0x1D, 0x00, 0x00, 0xC0, 0x3F}));
  ASSERT_EQ(fields.size(), 1U);
  EXPECT_EQ(fields[0].number, 3U);
  EXPECT_EQ(fields[0].type, WireType::kFixed32);
  EXPECT_EQ(fields[0].bytes.size(), 4U);
  EXPECT_EQ(ProtoReader::toFixed32(fields[0]), 0x3FC00000U);
  EXPECT_FLOAT_EQ(ProtoReader::toFloat(fields[0]), 1.5F);
}

TEST(ProtoReaderTest, ReadsFixed64AndDouble) {
  // field 4, wire type 1: double 2.0 = 0x4000000000000000.
  const auto fields =
      readAll(bytes({0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40}));
  ASSERT_EQ(fields.size(), 1U);
  EXPECT_EQ(fields[0].number, 4U);
  EXPECT_EQ(fields[0].type, WireType::kFixed64);
  EXPECT_EQ(ProtoReader::toFixed64(fields[0]), 0x4000000000000000ULL);
  EXPECT_DOUBLE_EQ(ProtoReader::toDouble(fields[0]), 2.0);
}

TEST(ProtoReaderTest, ReadsLengthDelimitedStringsAndBytes) {
  // field 5: "abc"; field 6: empty.
  const auto fields = readAll(bytes({0x2A, 0x03, 'a', 'b', 'c', 0x32, 0x00}));
  ASSERT_EQ(fields.size(), 2U);
  EXPECT_EQ(fields[0].type, WireType::kLengthDelimited);
  EXPECT_EQ(ProtoReader::toString(fields[0]), "abc");
  EXPECT_EQ(ProtoReader::toBytes(fields[0]).size(), 3U);
  EXPECT_EQ(ProtoReader::toString(fields[1]), "");
  EXPECT_TRUE(ProtoReader::toBytes(fields[1]).empty());
}

TEST(ProtoReaderTest, ReadsPackedDoubles) {
  // field 6: two doubles 2.0 and -0.5 (0xBFE0000000000000).
  const auto fields =
      readAll(bytes({0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBF}));
  ASSERT_EQ(fields.size(), 1U);
  std::vector<double> values;
  ProtoReader::packedDoubles(fields[0], values);
  ASSERT_EQ(values.size(), 2U);
  EXPECT_DOUBLE_EQ(values[0], 2.0);
  EXPECT_DOUBLE_EQ(values[1], -0.5);
}

TEST(ProtoReaderTest, PackedDoublesAcceptUnpackedElements) {
  // The same repeated field written unpacked: two fixed64 records.
  const auto fields =
      readAll(bytes({0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x31,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBF}));
  ASSERT_EQ(fields.size(), 2U);
  std::vector<double> values;
  ProtoReader::packedDoubles(fields[0], values);
  ProtoReader::packedDoubles(fields[1], values);
  ASSERT_EQ(values.size(), 2U);
  EXPECT_DOUBLE_EQ(values[0], 2.0);
  EXPECT_DOUBLE_EQ(values[1], -0.5);
}

TEST(ProtoReaderTest, ReadsPackedFixed32) {
  // field 8: fixed32 values 1 and 0x01020304.
  const auto fields = readAll(
      bytes({0x42, 0x08, 0x01, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01}));
  ASSERT_EQ(fields.size(), 1U);
  std::vector<uint32_t> values;
  ProtoReader::packedUint32(fields[0], values);
  ASSERT_EQ(values.size(), 2U);
  EXPECT_EQ(values[0], 1U);
  EXPECT_EQ(values[1], 0x01020304U);
  // An unpacked element appends too.
  const auto single = readAll(bytes({0x45, 0x07, 0x00, 0x00, 0x00}));
  ProtoReader::packedUint32(single[0], values);
  EXPECT_EQ(values.back(), 7U);
}

TEST(ProtoReaderTest, ReadsPackedVarints) {
  // field 1: varints 1, 300, 0.
  const auto fields = readAll(bytes({0x0A, 0x04, 0x01, 0xAC, 0x02, 0x00}));
  ASSERT_EQ(fields.size(), 1U);
  std::vector<uint64_t> values;
  ProtoReader::packedVarints(fields[0], values);
  ASSERT_EQ(values.size(), 3U);
  EXPECT_EQ(values[0], 1U);
  EXPECT_EQ(values[1], 300U);
  EXPECT_EQ(values[2], 0U);
  const auto single = readAll(bytes({0x08, 0x05}));
  ProtoReader::packedVarints(single[0], values);
  EXPECT_EQ(values.back(), 5U);
}

TEST(ProtoReaderTest, NestedMessagesAreNewReaders) {
  // field 7 = { field 1 = 1, field 2 = "x" }, then field 1 = 9 outside.
  const auto outer =
      readAll(bytes({0x3A, 0x05, 0x08, 0x01, 0x12, 0x01, 'x', 0x08, 0x09}));
  ASSERT_EQ(outer.size(), 2U);
  EXPECT_EQ(outer[0].number, 7U);
  EXPECT_EQ(outer[1].varint, 9U);
  ProtoReader inner(ProtoReader::toBytes(outer[0]));
  ProtoField field;
  ASSERT_TRUE(inner.next(field));
  EXPECT_EQ(field.number, 1U);
  EXPECT_EQ(field.varint, 1U);
  ASSERT_TRUE(inner.next(field));
  EXPECT_EQ(field.number, 2U);
  EXPECT_EQ(ProtoReader::toString(field), "x");
  EXPECT_FALSE(inner.next(field));
}

TEST(ProtoReaderTest, UnknownFieldsOfEveryWireTypeAreSkipped) {
  // field 99 varint (tag 0x98 0x06), field 100 fixed64 (tag 0xA1 0x06),
  // field 101 length-delimited (tag 0xAA 0x06), field 102 fixed32
  // (tag 0xB5 0x06), then the known field 1 = 42.
  const auto fields = readAll(bytes({
      0x98, 0x06, 0x7F,                             // 99
      0xA1, 0x06, 1,    2,    3,    4, 5, 6, 7, 8,  // 100
      0xAA, 0x06, 0x02, 0xFF, 0xFF,                 // 101
      0xB5, 0x06, 1,    2,    3,    4,              // 102
      0x08, 42,                                     // 1
  }));
  ASSERT_EQ(fields.size(), 5U);
  EXPECT_EQ(fields[0].number, 99U);
  EXPECT_EQ(fields[1].number, 100U);
  EXPECT_EQ(fields[2].number, 101U);
  EXPECT_EQ(fields[3].number, 102U);
  EXPECT_EQ(fields[4].number, 1U);
  EXPECT_EQ(fields[4].varint, 42U);
}

TEST(ProtoReaderTest, TruncatedVarintThrows) {
  const auto message = bytes({0x08, 0xAC});  // continuation bit, no more
  EXPECT_THROW(readAll(message), std::runtime_error);
  const auto tagOnly = bytes({0x08});
  EXPECT_THROW(readAll(tagOnly), std::runtime_error);
}

TEST(ProtoReaderTest, OverlongVarintThrows) {
  const auto message = bytes(
      {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01});
  EXPECT_THROW(readAll(message), std::runtime_error);
}

TEST(ProtoReaderTest, TruncatedFixedWidthFieldsThrow) {
  EXPECT_THROW(readAll(bytes({0x1D, 0x00, 0x00})), std::runtime_error);
  EXPECT_THROW(readAll(bytes({0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})),
               std::runtime_error);
}

TEST(ProtoReaderTest, TruncatedLengthDelimitedFieldThrows) {
  EXPECT_THROW(readAll(bytes({0x2A, 0x0A, 'a', 'b'})), std::runtime_error);
  EXPECT_THROW(readAll(bytes({0x2A})), std::runtime_error);
}

TEST(ProtoReaderTest, GroupsAndFieldZeroAreRejected) {
  EXPECT_THROW(readAll(bytes({0x0B})), std::runtime_error);  // start group
  EXPECT_THROW(readAll(bytes({0x0C})), std::runtime_error);  // end group
  EXPECT_THROW(readAll(bytes({0x00, 0x00})), std::runtime_error);  // field 0
}

TEST(ProtoReaderTest, ConvertersCheckTheWireType) {
  const auto fields = readAll(bytes({0x08, 0x01, 0x12, 0x01, 'x'}));
  ASSERT_EQ(fields.size(), 2U);
  EXPECT_THROW((void)ProtoReader::toDouble(fields[0]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toFloat(fields[0]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toFixed32(fields[0]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toFixed64(fields[0]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toString(fields[0]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toBytes(fields[0]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toSint(fields[1]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toInt(fields[1]), std::runtime_error);
  EXPECT_THROW((void)ProtoReader::toBool(fields[1]), std::runtime_error);
  std::vector<double> doubles;
  EXPECT_THROW(ProtoReader::packedDoubles(fields[0], doubles),
               std::runtime_error);
  std::vector<uint32_t> ints;
  EXPECT_THROW(ProtoReader::packedUint32(fields[0], ints), std::runtime_error);
}

TEST(ProtoReaderTest, PackedFieldsWithOddLengthThrow) {
  const auto doubles = readAll(bytes({0x32, 0x03, 1, 2, 3}));
  std::vector<double> values;
  EXPECT_THROW(ProtoReader::packedDoubles(doubles[0], values),
               std::runtime_error);
  std::vector<uint32_t> ints;
  EXPECT_THROW(ProtoReader::packedUint32(doubles[0], ints), std::runtime_error);
  // A packed varint list ending inside a varint.
  const auto varints = readAll(bytes({0x0A, 0x01, 0xAC}));
  std::vector<uint64_t> list;
  EXPECT_THROW(ProtoReader::packedVarints(varints[0], list),
               std::runtime_error);
}

TEST(ProtoReaderTest, ErrorMessagesNameTheField) {
  try {
    readAll(bytes({0x2A, 0x0A, 'a'}));
    FAIL() << "expected an exception";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find('5'), std::string::npos);
  }
}

}  // namespace camelot

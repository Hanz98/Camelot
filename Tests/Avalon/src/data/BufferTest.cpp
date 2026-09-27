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
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

#include "Avalon/src/data/buffers/Buffer.h"
#include "Avalon/src/data/buffers/UniformBuffer.h"
#include "Avalon/src/main/Avalon.h"

namespace avalon {

class BufferTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }
  [[nodiscard]] std::shared_ptr<VmaAllocatorWrapper> allocator() const {
    return avalon.getAllocator();
  }
};

TEST_F(BufferTest, HostVisibleBufferRoundTripsData) {
  Buffer buffer(allocator(), 64, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  ASSERT_TRUE(buffer.isValid());
  EXPECT_TRUE(buffer.isHostVisible());
  EXPECT_EQ(buffer.size(), 64U);
  EXPECT_EQ(buffer.usage(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  ASSERT_EQ(buffer.mapped().size(), 64U);

  const std::array<float, 4> values = {1.0F, 2.0F, 3.0F, 4.0F};
  buffer.write(std::span<const float>(values), 16);
  std::array<float, 4> readBack = {};
  std::memcpy(readBack.data(), buffer.mapped().subspan(16, 16).data(), 16);
  EXPECT_EQ(readBack, values);
}

TEST_F(BufferTest, WriteRejectsRangesOutsideTheBuffer) {
  Buffer buffer(allocator(), 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  const std::array<std::byte, 8> bytes = {};
  EXPECT_NO_THROW(buffer.write(bytes, 8));
  EXPECT_THROW(buffer.write(bytes, 9), std::runtime_error);
  EXPECT_THROW(buffer.write(bytes, 100), std::runtime_error);
}

TEST_F(BufferTest, DeviceLocalBufferIsNotWritable) {
  Buffer buffer(allocator(), 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false);
  ASSERT_TRUE(buffer.isValid());
  EXPECT_FALSE(buffer.isHostVisible());
  EXPECT_TRUE(buffer.mapped().empty());
  const std::array<std::byte, 8> bytes = {};
  EXPECT_THROW(buffer.write(bytes), std::runtime_error);
}

TEST_F(BufferTest, RejectsInvalidArguments) {
  EXPECT_THROW(Buffer(nullptr, 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
               std::runtime_error);
  EXPECT_THROW(Buffer(allocator(), 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
               std::runtime_error);
}

TEST_F(BufferTest, MoveTransfersOwnership) {
  Buffer first(allocator(), 32, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  const VkBuffer handle = first.get();
  Buffer second(std::move(first));
  EXPECT_EQ(second.get(), handle);
  EXPECT_EQ(second.size(), 32U);
  EXPECT_FALSE(first.isValid());  // NOLINT(bugprone-use-after-move)
  Buffer third;
  third = std::move(second);
  EXPECT_EQ(third.get(), handle);
  EXPECT_FALSE(second.isValid());  // NOLINT(bugprone-use-after-move)
}

TEST_F(BufferTest, UniformBufferHasOneBufferPerFrame) {
  struct Ubo {
    std::array<float, 4> values;
  };
  UniformBuffer ubo(allocator(), sizeof(Ubo), 3);
  EXPECT_EQ(ubo.frames(), 3U);
  EXPECT_EQ(ubo.size(), sizeof(Ubo));
  EXPECT_NE(ubo.buffer(0), ubo.buffer(1));
  EXPECT_NE(ubo.buffer(1), ubo.buffer(2));
  const Ubo value = {{5.0F, 6.0F, 7.0F, 8.0F}};
  EXPECT_NO_THROW(ubo.write(1, value));
  EXPECT_THROW(ubo.write(3, value), std::out_of_range);
  const VkDescriptorBufferInfo info = ubo.descriptorInfo(2);
  EXPECT_EQ(info.buffer, ubo.buffer(2));
  EXPECT_EQ(info.range, sizeof(Ubo));
  EXPECT_THROW(UniformBuffer(allocator(), 16, 0), std::runtime_error);
}

}  // namespace avalon

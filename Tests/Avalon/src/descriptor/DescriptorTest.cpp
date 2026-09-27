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

#include <memory>
#include <utility>
#include <vector>

#include "Avalon/src/data/buffers/UniformBuffer.h"
#include "Avalon/src/descriptor/DescriptorPool.h"
#include "Avalon/src/descriptor/DescriptorSetLayout.h"
#include "Avalon/src/main/Avalon.h"

namespace avalon {

class DescriptorTest : public testing::Test {
 protected:
  Avalon avalon;
  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }
  [[nodiscard]] std::shared_ptr<Device> device() const {
    return avalon.getDevice();
  }
};

TEST_F(DescriptorTest, LayoutDescribesItsBindings) {
  DescriptorSetLayout layout(
      device(),
      {DescriptorSetLayout::uniformBuffer(0, VK_SHADER_STAGE_VERTEX_BIT),
       DescriptorSetLayout::combinedImageSampler(
           1, VK_SHADER_STAGE_FRAGMENT_BIT)});
  EXPECT_NE(layout.get(), VK_NULL_HANDLE);
  ASSERT_EQ(layout.bindings().size(), 2U);
  EXPECT_EQ(layout.bindings()[0].descriptorType,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  EXPECT_EQ(layout.bindings()[1].descriptorType,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  EXPECT_EQ(layout.bindings()[1].binding, 1U);

  const VkDescriptorSetLayout handle = layout.get();
  DescriptorSetLayout moved(std::move(layout));
  EXPECT_EQ(moved.get(), handle);
  EXPECT_EQ(layout.get(), VK_NULL_HANDLE);  // NOLINT(bugprone-use-after-move)
}

TEST_F(DescriptorTest, PoolAllocatesAndWritesSets) {
  DescriptorSetLayout layout(device(), {DescriptorSetLayout::uniformBuffer(
                                           0, VK_SHADER_STAGE_VERTEX_BIT)});
  DescriptorPool pool(
      device(),
      {{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 2}}, 2);
  EXPECT_NE(pool.get(), VK_NULL_HANDLE);
  const std::vector<VkDescriptorSet> sets = pool.allocate(layout.get(), 2);
  ASSERT_EQ(sets.size(), 2U);
  EXPECT_NE(sets[0], VK_NULL_HANDLE);
  EXPECT_NE(sets[1], VK_NULL_HANDLE);

  UniformBuffer ubo(avalon.getAllocator(), 64, 2);
  EXPECT_NO_THROW(pool.writeUniformBuffer(sets[0], 0, ubo.descriptorInfo(0)));
  EXPECT_NO_THROW(pool.writeUniformBuffer(sets[1], 0, ubo.descriptorInfo(1)));
  EXPECT_NO_THROW(pool.reset());
  EXPECT_EQ(pool.allocate(layout.get(), 1).size(), 1U);
}

TEST_F(DescriptorTest, RendererExposesTheCameraSetLayout) {
  EXPECT_NE(avalon.getRenderer()->getCameraSetLayout(), VK_NULL_HANDLE);
}

}  // namespace avalon

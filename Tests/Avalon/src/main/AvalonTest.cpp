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

#include <Avalon/src/main/Avalon.h>
#include <gtest/gtest.h>

TEST(AvalonTest, InitCreatesAllObjects) {
  Avalon avalon;
  EXPECT_FALSE(avalon.isInitialized());

  ASSERT_NO_THROW(avalon.init());
  EXPECT_TRUE(avalon.isInitialized());
  ASSERT_NE(avalon.getWindow(), nullptr);
  EXPECT_NE(avalon.getWindow()->getWindow(), nullptr);
  ASSERT_NE(avalon.getDevice(), nullptr);
  EXPECT_NE(avalon.getDevice()->getDevice(), VK_NULL_HANDLE);

  avalon.cleanUp();
  EXPECT_FALSE(avalon.isInitialized());
  EXPECT_EQ(avalon.getWindow(), nullptr);
  EXPECT_EQ(avalon.getDevice(), nullptr);

  avalon.cleanUp();  // idempotent
  EXPECT_FALSE(avalon.isInitialized());
}

TEST(AvalonTest, CanBeInitializedTwice) {
  Avalon avalon;
  ASSERT_NO_THROW(avalon.init());
  avalon.cleanUp();
  ASSERT_NO_THROW(avalon.init());
  EXPECT_TRUE(avalon.isInitialized());
}

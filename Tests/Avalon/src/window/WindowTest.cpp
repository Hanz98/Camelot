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

#include <Avalon/src/window/Window.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>

TEST(WindowTest, DefaultConstructorCreatesWindow) {
  Window window;
  ASSERT_NE(window.getWindow(), nullptr)
      << "getWindow() should return a valid pointer after construction.";
  EXPECT_EQ(window.getWidth(), 800);
  EXPECT_EQ(window.getHeight(), 600);
  EXPECT_EQ(glfwGetWindowUserPointer(window.getWindow()), &window);
}

TEST(WindowTest, CleanupIsIdempotent) {
  Window window;
  ASSERT_NE(window.getWindow(), nullptr);

  window.cleanUp();
  EXPECT_EQ(window.getWindow(), nullptr)
      << "After cleanup, getWindow() should return nullptr.";

  window.cleanUp();
  EXPECT_EQ(window.getWindow(), nullptr);
}

TEST(WindowTest, MoveConstructorTransfersOwnership) {
  Window window;
  GLFWwindow* originalPtr = window.getWindow();
  ASSERT_NE(originalPtr, nullptr);

  Window moved(std::move(window));
  EXPECT_EQ(window.getWindow(), nullptr)
      << "The moved-from window should have a null pointer.";
  EXPECT_EQ(window.getWidth(), 0);
  EXPECT_EQ(window.getHeight(), 0);
  EXPECT_EQ(moved.getWindow(), originalPtr)
      << "The moved-to window should hold the original pointer.";
  EXPECT_EQ(moved.getWidth(), 800);
  EXPECT_EQ(moved.getHeight(), 600);
}

TEST(WindowTest, MoveAssignmentTransfersOwnership) {
  Window window;
  GLFWwindow* originalPtr = window.getWindow();
  ASSERT_NE(originalPtr, nullptr);

  Window target;
  target = std::move(window);
  EXPECT_EQ(window.getWindow(), nullptr)
      << "After move assignment, the original window should be empty.";
  EXPECT_EQ(target.getWindow(), originalPtr)
      << "The new window should hold the pointer from the moved window.";
  EXPECT_EQ(target.getWidth(), 800);
  EXPECT_EQ(target.getHeight(), 600);
}

TEST(WindowTest, FramebufferSizeIsNonZeroAndNotResizedInitially) {
  Window window;
  const auto [width, height] = window.getFramebufferSize();
  EXPECT_GT(width, 0U);
  EXPECT_GT(height, 0U);
  EXPECT_FALSE(window.shouldClose());
  window.pollEvents();
  EXPECT_FALSE(window.wasResized());
  EXPECT_FALSE(window.consumeResized());
}

TEST(WindowTest, FramebufferSizeIsZeroAfterCleanup) {
  Window window;
  window.cleanUp();
  const auto [width, height] = window.getFramebufferSize();
  EXPECT_EQ(width, 0U);
  EXPECT_EQ(height, 0U);
  EXPECT_TRUE(window.shouldClose());
}

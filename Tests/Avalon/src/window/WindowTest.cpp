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
#include <gtest/gtest.h>

#include <utility>

// Test that the window initializes correctly and that get() returns a valid
// pointer.
TEST(WindowTest, Initialization) {
  Window window;
  bool initSuccess = window.init(800, 600, "Test Window");
  EXPECT_TRUE(initSuccess);
  EXPECT_NE(window.getWindow(), nullptr)
      << "get() should return a valid pointer after initialization.";
}

// Test that cleanUp() correctly destroys the window and sets the pointer to
// nullptr.
TEST(WindowTest, Cleanup) {
  Window window;
  ASSERT_TRUE(window.init(800, 600, "Test Window"));
  window.cleanUp();
  EXPECT_EQ(window.getWindow(), nullptr)
      << "After cleanup, get() should return nullptr.";

  // Calling cleanup a second time should be safe.
  window.cleanUp();
  EXPECT_EQ(window.getWindow(), nullptr);
}

// Test the move constructor by verifying that after moving, the original window
// loses ownership.
TEST(WindowTest, MoveConstructor) {
  Window original;
  ASSERT_TRUE(original.init(800, 600, "Test Window"));
  GLFWwindow* originalPtr = original.getWindow();

  Window moved(std::move(original));
  EXPECT_EQ(original.getWindow(), nullptr)
      << "The moved-from window should have a null pointer.";
  EXPECT_EQ(moved.getWindow(), originalPtr)
      << "The moved-to window should hold the original pointer.";
}

// Test the move assignment operator.
TEST(WindowTest, MoveAssignment) {
  Window window1;
  ASSERT_TRUE(window1.init(800, 600, "Test Window 1"));
  GLFWwindow* window1Ptr = window1.getWindow();

  Window window2;
  window2 = std::move(window1);
  EXPECT_EQ(window1.getWindow(), nullptr)
      << "After move assignment, the original window should be empty.";
  EXPECT_EQ(window2.getWindow(), window1Ptr)
      << "The new window should hold the pointer from the moved window.";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

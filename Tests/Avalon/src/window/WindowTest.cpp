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
#include <Avalon/interface/window/IGlfWrapper.h>
#include <Avalon/src/window/Window.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>

// Test that the window initializes correctly and that get() returns a valid
// pointer.

struct GLFWwindow {
  int dummy;
};

class FakeGlfWrapper : public IGlfWrapper {
 private:
  GLFWwindow* ptr;

 public:
  int init() override {
    ptr = new GLFWwindow();
    return 0;
  }
  void terminate() override {}
  void windowHint(int hint, int value) override {}
  GLFWwindow* createWindow(int width, int height, const char* title,
                           GLFWmonitor* monitor, GLFWwindow* share) override {
    return ptr;
  }
  void destroyWindow(GLFWwindow* window) override { delete ptr; }
  void setWindowUserPointer(GLFWwindow* window, void* pointer) override {}
};

class WindowTest : public testing::Test {
 public:
  std::shared_ptr<FakeGlfWrapper> m_glfWrapper =
      std::make_shared<FakeGlfWrapper>();
  Window window = Window(m_glfWrapper);
};

TEST_F(WindowTest, Initialization) {
  bool initSuccess = window.init(800, 600, "Test Window");
  EXPECT_TRUE(initSuccess);
  EXPECT_NE(window.getWindow(), nullptr)
      << "get() should return a valid pointer after initialization.";
}

// Test that cleanUp() correctly destroys the window and sets the pointer to
// nullptr.
TEST_F(WindowTest, Cleanup) {
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
TEST_F(WindowTest, MoveConstructor) {
  ASSERT_TRUE(window.init(800, 600, "Test Window"));
  GLFWwindow* originalPtr = window.getWindow();

  Window moved(std::move(window));
  EXPECT_EQ(window.getWindow(), nullptr)
      << "The moved-from window should have a null pointer.";
  EXPECT_EQ(moved.getWindow(), originalPtr)
      << "The moved-to window should hold the original pointer.";
}

// Test the move assignment operator.
TEST_F(WindowTest, MoveAssignment) {
  ASSERT_TRUE(window.init(800, 600, "Test Window 1"));
  GLFWwindow* window1Ptr = window.getWindow();

  Window window2(m_glfWrapper);
  window2 = std::move(window);
  EXPECT_EQ(window.getWindow(), nullptr)
      << "After move assignment, the original window should be empty.";
  EXPECT_EQ(window2.getWindow(), window1Ptr)
      << "The new window should hold the pointer from the moved window.";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

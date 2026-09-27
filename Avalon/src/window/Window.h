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

#ifndef AVALON_SRC_WINDOW_WINDOW_H_
#define AVALON_SRC_WINDOW_WINDOW_H_

//  #include <pch.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <string>
#include <utility>

#include "Avalon/interface/window/IGlfWrapper.h"

namespace avalon {

class Window {
 public:
  // Mouse callbacks, dispatched from the GLFW callbacks on pollEvents().
  // Any hook may be left empty.
  struct InputHooks {
    std::function<void(int button, int action, int mods, double x, double y)>
        onMouseButton;
    std::function<void(double x, double y)> onCursorMove;
    std::function<void(double dx, double dy)> onScroll;
  };

 private:
  GLFWwindow* m_pWindow{nullptr};
  InputHooks m_hooks;
  std::pair<uint16_t, uint16_t> m_dimensions;
  // True while this object holds one reference on the GLFW library. GLFW is
  // terminated only when the last Window releases its reference.
  bool m_ownsGlfwRef{false};

 public:
  explicit Window();
  Window(Window&&) noexcept;
  Window(const Window&) = delete;
  Window& operator=(Window&&) noexcept;
  Window& operator=(const Window&) = delete;

  virtual ~Window();
  void cleanUp();

  [[nodiscard]] GLFWwindow* getWindow() const;

  [[nodiscard]] uint16_t getWidth() const;
  [[nodiscard]] uint16_t getHeight() const;

  // Size of the framebuffer in pixels (may differ from the window size on
  // HiDPI displays). Both are 0 while the window is minimised.
  [[nodiscard]] std::pair<uint32_t, uint32_t> getFramebufferSize() const;

  [[nodiscard]] bool shouldClose() const;
  void pollEvents() const;
  void waitEvents() const;

  // Set by the GLFW framebuffer-size callback; cleared by consumeResized().
  [[nodiscard]] bool wasResized() const;
  bool consumeResized();

  void setInputHooks(InputHooks hooks);
  [[nodiscard]] const InputHooks& getInputHooks() const { return m_hooks; }

  // Current cursor position in window coordinates.
  [[nodiscard]] std::pair<double, double> getCursorPosition() const;

 private:
  bool initialize(int width, int height, const std::string& title);
  void releaseGlfw();
  static void framebufferSizeCallback(GLFWwindow* window, int width,
                                      int height);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void cursorPosCallback(GLFWwindow* window, double x, double y);
  static void scrollCallback(GLFWwindow* window, double dx, double dy);

  bool m_resized = false;
};

}  // namespace avalon

#endif  // AVALON_SRC_WINDOW_WINDOW_H_

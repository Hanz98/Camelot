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
#include <Avalon/interface/window/IGlfWrapper.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <string>

class Window {
 private:
  std::shared_ptr<IGlfWrapper> m_glfWrapper;
  GLFWwindow* m_pWindow;

 public:
  explicit Window(std::shared_ptr<IGlfWrapper> glfWrapper = nullptr);
  Window(Window&&);
  Window(const Window&) = delete;
  Window& operator=(Window&&);
  Window& operator=(const Window&) = delete;

  ~Window();
  void cleanUp();

  bool init(int width, int height, const std::string& title);

  GLFWwindow* getWindow() const;
};

#endif  // AVALON_SRC_WINDOW_WINDOW_H_

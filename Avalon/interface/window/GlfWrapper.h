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

#ifndef AVALON_INTERFACE_WINDOW_GLFWRAPPER_H_
#define AVALON_INTERFACE_WINDOW_GLFWRAPPER_H_

#include <GLFW/glfw3.h>

#include "IGlfWrapper.h"

class GlfWrapper : public IGlfWrapper {
 public:
  int init() override;
  void terminate() override;
  void windowHint(int hint, int value) override;
  GLFWwindow* createWindow(int width, int height, const char* title,
                           GLFWmonitor* monitor, GLFWwindow* share) override;
  void destroyWindow(GLFWwindow* window) override;
  void setWindowUserPointer(GLFWwindow* window, void* pointer) override;
};

#endif  // AVALON_INTERFACE_WINDOW_GLFWRAPPER_H_

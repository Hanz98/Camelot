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

#ifndef AVALON_INTERFACE_WINDOW_IGLFWRAPPER_H_
#define AVALON_INTERFACE_WINDOW_IGLFWRAPPER_H_

struct GLFWwindow;
struct GLFWmonitor;

class IGlfWrapper {
 public:
  IGlfWrapper() = default;
  IGlfWrapper(IGlfWrapper&&) = delete;
  IGlfWrapper(const IGlfWrapper&) = delete;

  IGlfWrapper& operator=(IGlfWrapper&&) = delete;
  IGlfWrapper& operator=(const IGlfWrapper&) = delete;

  virtual ~IGlfWrapper() = default;
  virtual int init() = 0;
  virtual void terminate() = 0;
  virtual void windowHint(int hint, int value) = 0;
  virtual GLFWwindow* createWindow(int width, int height, const char* title,
                                   GLFWmonitor* monitor, GLFWwindow* share) = 0;
  virtual void destroyWindow(GLFWwindow* window) = 0;
  virtual void setWindowUserPointer(GLFWwindow* window, void* pointer) = 0;
};

#endif  // AVALON_INTERFACE_WINDOW_IGLFWRAPPER_H_

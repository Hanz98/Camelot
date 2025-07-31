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

#include "GlfWrapper.h"

namespace avalon {

int GlfWrapper::init() { return glfwInit(); }

void GlfWrapper::terminate() { glfwTerminate(); }

void GlfWrapper::windowHint(int hint, int value) {
  glfwWindowHint(hint, value);
}

GLFWwindow* GlfWrapper::createWindow(int width, int height, const char* title,
                                     GLFWmonitor* monitor, GLFWwindow* share) {
  return glfwCreateWindow(width, height, title, monitor, share);
}

void GlfWrapper::destroyWindow(GLFWwindow* window) {
  glfwDestroyWindow(window);
}

void GlfWrapper::setWindowUserPointer(GLFWwindow* window, void* pointer) {
  glfwSetWindowUserPointer(window, pointer);
}

}  // namespace avalon

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

#include "Window.h"

#include <string>

Window::Window() : m_pWindow(nullptr) {}

Window::Window(Window&& other) : m_pWindow(other.m_pWindow) {
  other.m_pWindow = nullptr;
}

Window& Window::operator=(Window&& other) {
  m_pWindow = other.m_pWindow;
  other.m_pWindow = nullptr;
  return *this;
}

Window::~Window() {
  cleanUp();
  glfwTerminate();
}

void Window::cleanUp() {
  if (m_pWindow) {
    glfwDestroyWindow(m_pWindow);
    m_pWindow = nullptr;
  }
}

bool Window::init(int width, int height, const std::string& title) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_pWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  if (m_pWindow == nullptr) {
    return false;
  }

  glfwSetWindowUserPointer(m_pWindow, this);
  return true;
}

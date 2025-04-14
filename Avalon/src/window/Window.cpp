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

#include <Avalon/interface/window/GlfWrapper.h>

#include <memory>
#include <string>
#include <utility>

Window::Window(std::shared_ptr<IGlfWrapper> glfWrapper)
    : m_pWindow(nullptr), m_glfWrapper(glfWrapper) {
  if (glfWrapper == nullptr) {
    m_glfWrapper = std::make_shared<GlfWrapper>();
  }
}

Window::Window(Window&& other)
    : m_pWindow(other.m_pWindow), m_glfWrapper(other.m_glfWrapper) {
  other.m_pWindow = nullptr;
  other.m_glfWrapper = nullptr;
}

Window& Window::operator=(Window&& other) {
  m_pWindow = other.m_pWindow;
  m_glfWrapper = other.m_glfWrapper;

  other.m_pWindow = nullptr;
  other.m_glfWrapper = nullptr;
  return *this;
}

Window::~Window() {
  cleanUp();
  glfwTerminate();
}

void Window::cleanUp() {
  if (m_pWindow) {
    m_glfWrapper->destroyWindow(m_pWindow);
    m_pWindow = nullptr;
  }
}

bool Window::init(int width, int height, const std::string& title) {
  m_dimensions = std::make_pair(static_cast<uint16_t>(width),
                                static_cast<uint16_t>(height));
  m_glfWrapper->init();
  m_glfWrapper->windowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_pWindow = m_glfWrapper->createWindow(width, height, title.c_str(), nullptr,
                                         nullptr);
  if (m_pWindow == nullptr) {
    return false;
  }

  m_glfWrapper->setWindowUserPointer(m_pWindow, this);
  return true;
}

GLFWwindow* Window::getWindow() const { return m_pWindow; }

uint16_t Window::getWidth() const { return m_dimensions.first; }

uint16_t Window::getHeight() const { return m_dimensions.second; }

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
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <utility>

#include "interface/window/IGlfWrapper.h"

namespace {
// Number of live Window objects that successfully called glfwInit().
int g_glfwRefCount = 0;

constexpr int kDefaultWindowWidth = 800;
constexpr int kDefaultWindowHeight = 600;
constexpr const char* kDefaultWindowTitle = "Avalon Window";
}  // namespace

Window::Window()
    : m_pWindow(nullptr), m_dimensions(0, 0), m_ownsGlfwRef(false) {
  initialize(kDefaultWindowWidth, kDefaultWindowHeight, kDefaultWindowTitle);
}

Window::Window(Window&& other) noexcept
    : m_pWindow(other.m_pWindow),
      m_dimensions(other.m_dimensions),
      m_ownsGlfwRef(other.m_ownsGlfwRef) {
  other.m_pWindow = nullptr;
  other.m_dimensions = {0, 0};
  other.m_ownsGlfwRef = false;
  if (m_pWindow != nullptr) {
    glfwSetWindowUserPointer(m_pWindow, this);
  }
}

Window& Window::operator=(Window&& other) noexcept {
  if (this != &other) {
    cleanUp();
    releaseGlfw();
    m_pWindow = other.m_pWindow;
    m_dimensions = other.m_dimensions;
    m_ownsGlfwRef = other.m_ownsGlfwRef;
    other.m_pWindow = nullptr;
    other.m_dimensions = {0, 0};
    other.m_ownsGlfwRef = false;
    if (m_pWindow != nullptr) {
      glfwSetWindowUserPointer(m_pWindow, this);
    }
  }
  return *this;
}

Window::~Window() {
  cleanUp();
  releaseGlfw();
}

void Window::releaseGlfw() {
  if (!m_ownsGlfwRef) {
    return;
  }
  m_ownsGlfwRef = false;
  if (--g_glfwRefCount == 0) {
    glfwTerminate();
  }
}

void Window::cleanUp() {
  if (m_pWindow) {
    glfwDestroyWindow(m_pWindow);
    m_pWindow = nullptr;
  }
}

bool Window::initialize(int width, int height, const std::string& title) {
  if (glfwInit() != GLFW_TRUE) {
    return false;
  }
  ++g_glfwRefCount;
  m_ownsGlfwRef = true;
  m_dimensions = std::make_pair(static_cast<uint16_t>(width),
                                static_cast<uint16_t>(height));
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_pWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

  if (m_pWindow == nullptr) {
    return false;
  }

  glfwSetWindowUserPointer(m_pWindow, this);
  glfwSetFramebufferSizeCallback(m_pWindow, &Window::framebufferSizeCallback);
  return true;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width,
                                     int height) {
  auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) {
    return;
  }
  self->m_resized = true;
  self->m_dimensions = std::make_pair(static_cast<uint16_t>(width),
                                      static_cast<uint16_t>(height));
}

std::pair<uint32_t, uint32_t> Window::getFramebufferSize() const {
  if (m_pWindow == nullptr) {
    return {0, 0};
  }
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(m_pWindow, &width, &height);
  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

bool Window::shouldClose() const {
  return m_pWindow == nullptr || glfwWindowShouldClose(m_pWindow) != 0;
}

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEvents() const { glfwWaitEvents(); }

bool Window::wasResized() const { return m_resized; }

bool Window::consumeResized() {
  const bool resized = m_resized;
  m_resized = false;
  return resized;
}

[[nodiscard]] GLFWwindow* Window::getWindow() const { return m_pWindow; }

uint16_t Window::getWidth() const { return m_dimensions.first; }

uint16_t Window::getHeight() const { return m_dimensions.second; }

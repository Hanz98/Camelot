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

#include "Surface.h"

#define GLFW_INCLUDE_VULKAN
#include <Avalon/src/device/Instance.h>
#include <Avalon/src/validation/CheckResult.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <memory>

#include "Window.h"

Surface::Surface(std::shared_ptr<Instance> instance,
                 std::shared_ptr<Window> window)
    : m_instance(instance), m_window(window), m_surface(VK_NULL_HANDLE) {}

Surface::~Surface() { cleanUp(); }

void Surface::init() {
  if (m_instance == nullptr || m_window == nullptr) {
    spdlog::error(
        "Cannot initialize Surface without valid instance and window pointer!");
    throw std::runtime_error(
        "Cannot initialize Surface without valid instance and window pointer!");
  }

  VK_CHECK_RESULT(glfwCreateWindowSurface(
      m_instance->getInstance(), m_window->getWindow(), nullptr, &m_surface));
}

void Surface::cleanUp() {
  if (m_surface != VK_NULL_HANDLE && m_instance != nullptr) {
    vkDestroySurfaceKHR(m_instance->getInstance(), m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }
}

const VkSurfaceKHR& Surface::getSurface() const { return m_surface; }

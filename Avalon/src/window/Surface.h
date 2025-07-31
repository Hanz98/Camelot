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

#ifndef AVALON_SRC_WINDOW_SURFACE_H_
#define AVALON_SRC_WINDOW_SURFACE_H_

#define GLFW_INCLUDE_VULKAN
#include <Avalon/src/device/Instance.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <memory>

#include "Window.h"

namespace avalon {

class Surface {
 private:
  VkSurfaceKHR m_surface;
  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<Window> m_window;

 public:
  Surface(std::shared_ptr<Instance>, std::shared_ptr<Window>);
  Surface(const Surface&) = delete;
  Surface& operator==(const Surface&) = delete;
  Surface(Surface&&) = delete;
  Surface& operator==(Surface&&) = delete;

  virtual ~Surface();

  const VkSurfaceKHR& getSurface() const;
  void init();
  void cleanUp();
};

}  // namespace avalon

#endif  //  AVALON_SRC_WINDOW_SURFACE_H_

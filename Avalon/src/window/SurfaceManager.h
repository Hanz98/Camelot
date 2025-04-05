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

#ifndef AVALON_SRC_WINDOW_SURFACEMANAGER_H_
#define AVALON_SRC_WINDOW_SURFACEMANAGER_H_

#include <Avalon/src/device/Instance.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "Surface.h"
#include "Window.h"

class SurfaceManager {
 private:
  std::shared_ptr<Instance> m_instance;

  std::vector<std::shared_ptr<Surface>> m_surfaces;
  std::shared_ptr<Window> m_window;

 public:
  SurfaceManager();
  SurfaceManager(const SurfaceManager&) = delete;
  SurfaceManager& operator=(const SurfaceManager&) = delete;
  SurfaceManager(SurfaceManager&&) = delete;
  SurfaceManager& operator=(SurfaceManager&&) = delete;
  ~SurfaceManager();

  void init();
  void cleanUp();

 private:
};

#endif  // AVALON_SRC_WINDOW_SURFACEMANAGER_H_

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

#include "SurfaceManager.h"

#include <Avalon/src/validation/CheckResult.h>

#include <memory>

SurfaceManager::SurfaceManager(std::shared_ptr<Instance> instance,
                               std::shared_ptr<Window> window)
    : m_window(window), m_instance(instance) {}

SurfaceManager::~SurfaceManager() { cleanUp(); }

void SurfaceManager::init() {
  m_surfaces.push_back(std::make_shared<Surface>(m_instance, m_window));

  for (auto surface : m_surfaces) {
    surface->init();
  }
}

void SurfaceManager::cleanUp() {
  for (auto surface : m_surfaces) {
    surface->cleanUp();
  }
  m_surfaces.clear();
}

std::shared_ptr<Surface> SurfaceManager::getSurface(int id) {
  if (m_surfaces.size() < id) {
    return nullptr;
  }

  return m_surfaces.at(id);
}

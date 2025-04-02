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

SurfaceManager::SurfaceManager() : m_window(nullptr) {}

SurfaceManager::~SurfaceManager() { cleanUp(); }

void SurfaceManager::init() {
  m_window = std::make_shared<Window>();

  m_window->init(400, 600, "Camelot");

  //    VK_CHECK_RESULT(glfwCreateWindowSurface(m_instance, ))
}

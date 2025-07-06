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

#include "ResourceDescriptor.h"

#include <memory>

std::shared_ptr<Resource::Descriptor> Resource::Descriptor::INSTANCE = nullptr;

std::shared_ptr<Resource::Descriptor> Resource::Descriptor::GetDescriptor() {
  if (INSTANCE == nullptr) {
    INSTANCE =
        std::shared_ptr<Resource::Descriptor>(new Resource::Descriptor());
  }
  return INSTANCE;
}

std::shared_ptr<Device> Resource::Descriptor::getDevice() const {
  return m_device;
}

std::shared_ptr<Instance> Resource::Descriptor::getInstance() const {
  return m_instance;
}

std::shared_ptr<Window> Resource::Descriptor::getWindow() const {
  return m_window;
}

std::shared_ptr<Surface> Resource::Descriptor::getSurface() const {
  return m_surface;
}

std::shared_ptr<SurfaceManager> Resource::Descriptor::getSurfaceManager()
    const {
  return m_surfaceManager;
}

std::shared_ptr<RenderPass> Resource::Descriptor::getRenderPass() const {
  return m_renderPass;
}

std::shared_ptr<SwapChainModel> Resource::Descriptor::getSwapchainModel()
    const {
  return m_swapchainModel;
}

std::shared_ptr<VmaAllocatorWrapper> Resource::Descriptor::getAllocator()
    const {
  return m_allocator;
}

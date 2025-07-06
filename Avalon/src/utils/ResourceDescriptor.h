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

#ifndef AVALON_SRC_UTILS_RESOURCEDESCRIPTOR_H_
#define AVALON_SRC_UTILS_RESOURCEDESCRIPTOR_H_
#include <Avalon/src/allocator/VmaAllocator.h>

#include <memory>

class Device;
class Instance;
class Window;
class Surface;
class SurfaceManager;
class RenderPass;
class SwapChainModel;

namespace Resource {
class Descriptor {
 private:
  static std::shared_ptr<Descriptor> INSTANCE;
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<Surface> m_surface;
  std::shared_ptr<SurfaceManager> m_surfaceManager;
  std::shared_ptr<RenderPass> m_renderPass;
  std::shared_ptr<SwapChainModel> m_swapchainModel;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  Descriptor() = default;

 public:
  static std::shared_ptr<Descriptor> GetDescriptor();

  std::shared_ptr<Device> getDevice() const;
  std::shared_ptr<Instance> getInstance() const;
  std::shared_ptr<Window> getWindow() const;
  std::shared_ptr<Surface> getSurface() const;
  std::shared_ptr<SurfaceManager> getSurfaceManager() const;
  std::shared_ptr<RenderPass> getRenderPass() const;
  std::shared_ptr<SwapChainModel> getSwapchainModel() const;
  std::shared_ptr<VmaAllocatorWrapper> getAllocator() const;
};

}  // namespace Resource

#endif  // AVALON_SRC_UTILS_RESOURCEDESCRIPTOR_H_

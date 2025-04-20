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
struct DescriptorInfo {
  std::shared_ptr<Device> device;
  std::shared_ptr<Instance> instance;
  std::shared_ptr<Window> window;
  std::shared_ptr<Surface> surface;
  std::shared_ptr<SurfaceManager> surfaceManager;
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<SwapChainModel> swapchainModel;
  std::shared_ptr<VmaAllocatorWrapper> allocator;
};

static std::shared_ptr<DescriptorInfo> Descriptor;
}  // namespace Resource

#endif  // AVALON_SRC_UTILS_RESOURCEDESCRIPTOR_H_

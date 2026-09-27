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

#ifndef AVALON_SRC_SYNC_FRAMESYNC_H_
#define AVALON_SRC_SYNC_FRAMESYNC_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "Avalon/src/device/Device.h"

namespace avalon {

// Synchronisation objects for N frames in flight:
//  - one "image available" semaphore and one fence per in-flight frame,
//  - one "render finished" semaphore per swapchain image (a present may still
//    be reading it when the same frame slot comes around again).
class FrameSync {
 private:
  std::shared_ptr<Device> m_device;
  std::vector<VkSemaphore> m_imageAvailable;
  std::vector<VkSemaphore> m_renderFinished;
  std::vector<VkFence> m_inFlight;

 public:
  FrameSync(std::shared_ptr<Device> device, uint32_t framesInFlight,
            uint32_t swapchainImageCount);
  FrameSync(const FrameSync&) = delete;
  FrameSync& operator=(const FrameSync&) = delete;
  FrameSync(FrameSync&&) = delete;
  FrameSync& operator=(FrameSync&&) = delete;
  ~FrameSync();

  void cleanUp();

  [[nodiscard]] uint32_t framesInFlight() const {
    return static_cast<uint32_t>(m_inFlight.size());
  }
  [[nodiscard]] VkSemaphore imageAvailable(uint32_t frame) const {
    return m_imageAvailable.at(frame);
  }
  [[nodiscard]] VkSemaphore renderFinished(uint32_t imageIndex) const {
    return m_renderFinished.at(imageIndex);
  }
  [[nodiscard]] VkFence inFlight(uint32_t frame) const {
    return m_inFlight.at(frame);
  }

  // Recreates the per-image semaphores when the swapchain image count changes.
  void resizeImageSemaphores(uint32_t swapchainImageCount);
};

}  // namespace avalon

#endif  // AVALON_SRC_SYNC_FRAMESYNC_H_

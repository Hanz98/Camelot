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

#include "FrameSync.h"

#include <Avalon/src/validation/CheckResult.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <utility>

FrameSync::FrameSync(std::shared_ptr<Device> device, uint32_t framesInFlight,
                     uint32_t swapchainImageCount)
    : m_device(std::move(device)) {
  if (m_device == nullptr) {
    spdlog::error("FrameSync: Device is not initialized.");
    throw std::runtime_error("FrameSync: Device is not initialized.");
  }
  if (framesInFlight == 0) {
    throw std::runtime_error("FrameSync: framesInFlight must be > 0.");
  }

  VkSemaphoreCreateInfo semInfo = {};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // first wait returns at once

  m_imageAvailable.resize(framesInFlight, VK_NULL_HANDLE);
  m_inFlight.resize(framesInFlight, VK_NULL_HANDLE);
  for (uint32_t i = 0; i < framesInFlight; ++i) {
    VK_CHECK_RESULT(vkCreateSemaphore(m_device->getDevice(), &semInfo, nullptr,
                                      &m_imageAvailable[i]));
    VK_CHECK_RESULT(vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr,
                                  &m_inFlight[i]));
  }
  resizeImageSemaphores(swapchainImageCount);
}

FrameSync::~FrameSync() { cleanUp(); }

void FrameSync::resizeImageSemaphores(uint32_t swapchainImageCount) {
  VkDevice device = m_device->getDevice();
  for (VkSemaphore sem : m_renderFinished) {
    vkDestroySemaphore(device, sem, nullptr);
  }
  m_renderFinished.assign(swapchainImageCount, VK_NULL_HANDLE);
  VkSemaphoreCreateInfo semInfo = {};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (auto& sem : m_renderFinished) {
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semInfo, nullptr, &sem));
  }
}

void FrameSync::cleanUp() {
  VkDevice device = m_device->getDevice();
  if (device == VK_NULL_HANDLE) {
    return;
  }
  for (VkSemaphore sem : m_renderFinished) {
    vkDestroySemaphore(device, sem, nullptr);
  }
  m_renderFinished.clear();
  for (VkSemaphore sem : m_imageAvailable) {
    vkDestroySemaphore(device, sem, nullptr);
  }
  m_imageAvailable.clear();
  for (VkFence fence : m_inFlight) {
    vkDestroyFence(device, fence, nullptr);
  }
  m_inFlight.clear();
}

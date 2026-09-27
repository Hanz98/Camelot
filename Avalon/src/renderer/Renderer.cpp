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

#include "Renderer.h"

#include <Avalon/src/validation/CheckResult.h>
#include <spdlog/spdlog.h>

#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

Renderer::Renderer(std::shared_ptr<Device> device,
                   std::shared_ptr<Window> window,
                   std::shared_ptr<SwapchainModel> swapchain)
    : m_device(std::move(device)),
      m_window(std::move(window)),
      m_swapchain(std::move(swapchain)),

      m_clearColor{0.05F, 0.05F, 0.08F, 1.0F} {
  if (m_device == nullptr || m_window == nullptr || m_swapchain == nullptr) {
    spdlog::error("Renderer: device, window or swapchain is not initialized.");
    throw std::runtime_error(
        "Renderer: device, window or swapchain is not initialized.");
  }
  m_renderPass = std::make_unique<RenderPass>(
      m_device, m_swapchain->getImageFormat(), m_device->getDepthFormat(),
      m_swapchain->getSamples());
  m_swapchain->createFramebuffers(*m_renderPass);
  m_commandPool = std::make_unique<CommandPool>(m_device);
  m_commandBuffers = m_commandPool->allocate(kFramesInFlight);
  m_sync = std::make_unique<FrameSync>(m_device, kFramesInFlight,
                                       m_swapchain->getImageCount());
}

Renderer::~Renderer() { cleanUp(); }

void Renderer::cleanUp() {
  if (m_device != nullptr) {
    m_device->waitIdle();
  }
  m_sync.reset();
  m_commandBuffers.clear();  // freed with the pool
  m_commandPool.reset();
  m_renderPass.reset();
}

void Renderer::setClearColor(float r, float g, float b, float a) {
  m_clearColor = {r, g, b, a};
}

bool Renderer::recreateSwapchain() {
  const auto [width, height] = m_window->getFramebufferSize();
  if (width == 0 || height == 0) {
    return false;  // minimised: nothing to render into
  }
  m_device->waitIdle();
  m_swapchain->recreate();
  m_sync->resizeImageSemaphores(m_swapchain->getImageCount());
  m_window->consumeResized();
  return true;
}

bool Renderer::drawFrame() {
  VkDevice device = m_device->getDevice();
  const VkFence fence = m_sync->inFlight(m_currentFrame);
  VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE,
                                  std::numeric_limits<uint64_t>::max()));

  uint32_t imageIndex = 0;
  VkResult acquire = vkAcquireNextImageKHR(
      device, m_swapchain->getSwapchain(), std::numeric_limits<uint64_t>::max(),
      m_sync->imageAvailable(m_currentFrame), VK_NULL_HANDLE, &imageIndex);
  if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return false;
  }
  if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
    spdlog::error("vkAcquireNextImageKHR failed with {}",
                  static_cast<int>(acquire));
    throw std::runtime_error("Failed to acquire swapchain image.");
  }

  // Only reset the fence once we know we will submit work that signals it.
  VK_CHECK_RESULT(vkResetFences(device, 1, &fence));

  VkCommandBuffer commandBuffer = m_commandBuffers.at(m_currentFrame);
  VK_CHECK_RESULT(vkResetCommandBuffer(commandBuffer, 0));
  recordCommandBuffer(commandBuffer, imageIndex);

  const std::array<VkSemaphore, 1> waitSemaphores = {
      m_sync->imageAvailable(m_currentFrame)};
  const std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  const std::array<VkSemaphore, 1> signalSemaphores = {
      m_sync->renderFinished(imageIndex)};

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submit.pWaitSemaphores = waitSemaphores.data();
  submit.pWaitDstStageMask = waitStages.data();
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffer;
  submit.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
  submit.pSignalSemaphores = signalSemaphores.data();
  VK_CHECK_RESULT(
      vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submit, fence));

  const std::array<VkSwapchainKHR, 1> swapchains = {
      m_swapchain->getSwapchain()};
  VkPresentInfoKHR present = {};
  present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
  present.pWaitSemaphores = signalSemaphores.data();
  present.swapchainCount = static_cast<uint32_t>(swapchains.size());
  present.pSwapchains = swapchains.data();
  present.pImageIndices = &imageIndex;

  VkResult presented = vkQueuePresentKHR(m_device->getPresentQueue(), &present);
  if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR ||
      m_window->wasResized()) {
    recreateSwapchain();
  } else if (presented != VK_SUCCESS) {
    spdlog::error("vkQueuePresentKHR failed with {}",
                  static_cast<int>(presented));
    throw std::runtime_error("Failed to present swapchain image.");
  }

  m_currentFrame = (m_currentFrame + 1) % kFramesInFlight;
  ++m_frameCount;
  return true;
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex) {
  VkCommandBufferBeginInfo begin = {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &begin));

  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color = {
      {m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]}};
  clearValues[1].depthStencil = {.depth = 1.0F, .stencil = 0};

  VkRenderPassBeginInfo passBegin = {};
  passBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  passBegin.renderPass = m_renderPass->getRenderPass();
  passBegin.framebuffer = m_swapchain->getFramebuffer(imageIndex);
  passBegin.renderArea.offset = {.x = 0, .y = 0};
  passBegin.renderArea.extent = m_swapchain->getExtent();
  passBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
  passBegin.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &passBegin, VK_SUBPASS_CONTENTS_INLINE);
  // Drawing goes here (T3+). For now the pass only clears.
  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

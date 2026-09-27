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

#include "Avalon/src/renderer/Renderer.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/pipeline/GraphicsPipeline.h"
#include "Avalon/src/shader/ShaderModule.h"
#include "Avalon/src/ui/UiContext.h"
#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

Renderer::Renderer(std::shared_ptr<Device> device,
                   std::shared_ptr<Window> window,
                   std::shared_ptr<SwapchainModel> swapchain,
                   std::shared_ptr<VmaAllocatorWrapper> allocator)
    : m_device(std::move(device)),
      m_window(std::move(window)),
      m_swapchain(std::move(swapchain)),
      m_allocator(std::move(allocator)),
      m_clearColor{0.05F, 0.05F, 0.08F, 1.0F} {
  if (m_device == nullptr || m_window == nullptr || m_swapchain == nullptr ||
      m_allocator == nullptr) {
    spdlog::error(
        "Renderer: device, window, swapchain or allocator is not initialized.");
    throw std::runtime_error(
        "Renderer: device, window, swapchain or allocator is not initialized.");
  }
  m_renderPass = std::make_unique<RenderPass>(
      m_device, m_swapchain->getImageFormat(), m_device->getDepthFormat(),
      m_swapchain->getSamples());
  m_swapchain->createFramebuffers(*m_renderPass);
  m_commandPool = std::make_unique<CommandPool>(m_device);
  m_commandBuffers = m_commandPool->allocate(kFramesInFlight);
  m_sync = std::make_unique<FrameSync>(m_device, kFramesInFlight,
                                       m_swapchain->getImageCount());
  m_pipelines = std::make_unique<PipelineManager>(m_device);
  createCameraResources();
  createPipelines();
}

void Renderer::createCameraResources() {
  m_cameraLayout = std::make_unique<DescriptorSetLayout>(
      m_device,
      std::vector<VkDescriptorSetLayoutBinding>{
          DescriptorSetLayout::uniformBuffer(
              0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)});
  m_descriptorPool = std::make_unique<DescriptorPool>(
      m_device,
      std::vector<VkDescriptorPoolSize>{
          {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           .descriptorCount = kFramesInFlight}},
      kFramesInFlight);
  m_cameraUbo = std::make_unique<UniformBuffer>(m_allocator, sizeof(CameraUbo),
                                                kFramesInFlight);
  m_cameraSets =
      m_descriptorPool->allocate(m_cameraLayout->get(), kFramesInFlight);
  for (uint32_t frame = 0; frame < kFramesInFlight; ++frame) {
    m_descriptorPool->writeUniformBuffer(m_cameraSets.at(frame), 0,
                                         m_cameraUbo->descriptorInfo(frame));
  }
}

void Renderer::addDrawable(std::shared_ptr<IDrawable> drawable) {
  if (drawable == nullptr) {
    spdlog::error("Renderer::addDrawable: drawable is null.");
    throw std::runtime_error("Renderer::addDrawable: drawable is null.");
  }
  m_drawables.push_back(std::move(drawable));
}

bool Renderer::removeDrawable(const std::shared_ptr<IDrawable>& drawable) {
  auto it = std::ranges::find(m_drawables, drawable);
  if (it == m_drawables.end()) {
    return false;
  }
  m_device->waitIdle();  // a command buffer may still reference it
  m_drawables.erase(it);
  return true;
}

void Renderer::addPrePass(std::shared_ptr<IPrePass> prePass) {
  if (prePass == nullptr) {
    spdlog::error("Renderer::addPrePass: pre-pass is null.");
    throw std::runtime_error("Renderer::addPrePass: pre-pass is null.");
  }
  m_prePasses.push_back(std::move(prePass));
}

bool Renderer::removePrePass(const std::shared_ptr<IPrePass>& prePass) {
  auto it = std::ranges::find(m_prePasses, prePass);
  if (it == m_prePasses.end()) {
    return false;
  }
  m_device->waitIdle();
  m_prePasses.erase(it);
  return true;
}

void Renderer::clearDrawables() {
  if (m_drawables.empty()) {
    return;
  }
  m_device->waitIdle();
  m_drawables.clear();
}

void Renderer::createPipelines() {
  m_pipelines->getOrCreate(kTrianglePipeline, [this](PipelineManager& pm) {
    const ShaderModule vert(m_device, shaders::get("triangle.vert"));
    const ShaderModule frag(m_device, shaders::get("triangle.frag"));
    return GraphicsPipelineBuilder(kTrianglePipeline)
        .addStage(vert)
        .addStage(frag)
        .setRenderPass(m_renderPass->getRenderPass())
        .setLayout(pm.getOrCreateLayout("empty").get())
        .setSamples(m_renderPass->getSamples())
        .setDepth(true, true)
        .build(m_device);
  });
}

Renderer::~Renderer() { cleanUp(); }

void Renderer::cleanUp() {
  if (m_device != nullptr) {
    m_device->waitIdle();
  }
  m_ui = nullptr;
  m_prePasses.clear();
  m_drawables.clear();
  m_pipelines.reset();
  m_cameraSets.clear();  // freed with the pool
  m_descriptorPool.reset();
  m_cameraUbo.reset();
  m_cameraLayout.reset();
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

  const VkExtent2D extent = m_swapchain->getExtent();
  m_camera.setAspect(extent.width, extent.height);
  m_cameraUbo->write(m_currentFrame, m_camera.ubo());

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

  for (const std::shared_ptr<IPrePass>& prePass : m_prePasses) {
    prePass->recordPrePass(commandBuffer, m_currentFrame);
  }

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
  FrameContext context;
  context.commandBuffer = commandBuffer;
  context.frameIndex = m_currentFrame;
  context.extent = m_swapchain->getExtent();
  context.renderPass = m_renderPass->getRenderPass();
  context.samples = m_renderPass->getSamples();
  context.cameraSet = m_cameraSets.at(m_currentFrame);
  context.cameraSetLayout = m_cameraLayout->get();
  context.pipelines = m_pipelines.get();
  if (m_drawablesInMainPass) {
    for (const std::shared_ptr<IDrawable>& drawable : m_drawables) {
      drawable->record(context);
    }
  }
  if (m_ui != nullptr) {
    m_ui->render(commandBuffer);
  }
  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

}  // namespace avalon

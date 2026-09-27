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

#include "Avalon/src/ui/RenderTarget.h"

#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

RenderTarget::RenderTarget(std::shared_ptr<Device> device,
                           std::shared_ptr<VmaAllocatorWrapper> allocator,
                           const UiContext* ui, uint32_t width, uint32_t height)
    : m_device(std::move(device)),
      m_allocator(std::move(allocator)),
      m_ui(ui),
      m_color(m_device, m_allocator),
      m_depth(m_device, m_allocator),
      m_sampler(m_device) {
  if (m_device == nullptr || m_allocator == nullptr || m_ui == nullptr) {
    spdlog::error("RenderTarget: device, allocator or UI context is null.");
    throw std::runtime_error(
        "RenderTarget: device, allocator or UI context is null.");
  }
  if (width == 0 || height == 0) {
    spdlog::error("RenderTarget: size must be positive.");
    throw std::runtime_error("RenderTarget: size must be positive.");
  }
  m_depthFormat = m_device->getDepthFormat();

  // Colour: cleared, kept, then sampled by the UI. Depth: cleared, dropped.
  std::array<VkAttachmentDescription, 2> attachments = {};
  attachments[0].format = kColorFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachments[1].format = m_depthFormat;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  const VkAttachmentReference colorRef = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  const VkAttachmentReference depthRef = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  // Previous frame's UI may still sample the image; the next UI pass waits
  // for our colour writes.
  const std::array<VkSubpassDependency, 2> dependencies = {
      VkSubpassDependency{
          .srcSubpass = VK_SUBPASS_EXTERNAL,
          .dstSubpass = 0,
          .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
          .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .dependencyFlags = 0},
      VkSubpassDependency{
          .srcSubpass = 0,
          .dstSubpass = VK_SUBPASS_EXTERNAL,
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
          .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
          .dependencyFlags = 0}};

  VkRenderPassCreateInfo passInfo = {};
  passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  passInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  passInfo.pAttachments = attachments.data();
  passInfo.subpassCount = 1;
  passInfo.pSubpasses = &subpass;
  passInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  passInfo.pDependencies = dependencies.data();
  VK_CHECK_RESULT(vkCreateRenderPass(m_device->getDevice(), &passInfo, nullptr,
                                     &m_renderPass));
  create(width, height);
}

RenderTarget::~RenderTarget() { cleanUp(); }

void RenderTarget::cleanUp() {
  if (m_device == nullptr) {
    return;
  }
  m_device->waitIdle();
  destroyImages();
  if (m_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;
  }
  m_sampler.cleanUp();
  m_device = nullptr;
}

void RenderTarget::destroyImages() {
  if (m_uiTexture != VK_NULL_HANDLE) {
    m_ui->unregisterTexture(m_uiTexture);
    m_uiTexture = VK_NULL_HANDLE;
  }
  if (m_framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(m_device->getDevice(), m_framebuffer, nullptr);
    m_framebuffer = VK_NULL_HANDLE;
  }
  m_color.cleanUp();
  m_depth.cleanUp();
}

void RenderTarget::create(uint32_t width, uint32_t height) {
  ImageCreateInfo color;
  color.width = static_cast<int>(width);
  color.height = static_cast<int>(height);
  color.mipLevels = 1;
  color.numSample = VK_SAMPLE_COUNT_1_BIT;
  color.format = kColorFormat;
  color.tiling = VK_IMAGE_TILING_OPTIMAL;
  color.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  color.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  color.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
  m_color.createImage(color);

  ImageCreateInfo depth = color;
  depth.format = m_depthFormat;
  depth.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  depth.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
  m_depth.createImage(depth);

  const std::array<VkImageView, 2> views = {m_color.getImageView(),
                                            m_depth.getImageView()};
  VkFramebufferCreateInfo fbInfo = {};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.renderPass = m_renderPass;
  fbInfo.attachmentCount = static_cast<uint32_t>(views.size());
  fbInfo.pAttachments = views.data();
  fbInfo.width = width;
  fbInfo.height = height;
  fbInfo.layers = 1;
  VK_CHECK_RESULT(vkCreateFramebuffer(m_device->getDevice(), &fbInfo, nullptr,
                                      &m_framebuffer));
  m_uiTexture = m_ui->registerTexture(m_sampler.get(), m_color.getImageView());
  m_width = width;
  m_height = height;
  ++m_generation;
}

bool RenderTarget::resize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0 || (width == m_width && height == m_height)) {
    return false;
  }
  m_device->waitIdle();
  destroyImages();
  create(width, height);
  return true;
}

void RenderTarget::beginPass(VkCommandBuffer commandBuffer,
                             const std::array<float, 4>& clearColor) const {
  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color = {
      {clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};
  clearValues[1].depthStencil = {.depth = 1.0F, .stencil = 0};
  VkRenderPassBeginInfo begin = {};
  begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin.renderPass = m_renderPass;
  begin.framebuffer = m_framebuffer;
  begin.renderArea.offset = {.x = 0, .y = 0};
  begin.renderArea.extent = extent();
  begin.clearValueCount = static_cast<uint32_t>(clearValues.size());
  begin.pClearValues = clearValues.data();
  vkCmdBeginRenderPass(commandBuffer, &begin, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderTarget::endPass(VkCommandBuffer commandBuffer) const {
  vkCmdEndRenderPass(commandBuffer);
}

}  // namespace avalon

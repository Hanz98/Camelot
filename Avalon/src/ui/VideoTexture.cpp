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

#include "Avalon/src/ui/VideoTexture.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Avalon/src/camera/Camera.h"
#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

VideoTexture::VideoTexture(std::shared_ptr<Device> device,
                           std::shared_ptr<VmaAllocatorWrapper> allocator,
                           const UiContext* ui, uint32_t width, uint32_t height,
                           uint32_t framesInFlight)
    : m_device(std::move(device)),
      m_allocator(std::move(allocator)),
      m_ui(ui),
      m_width(width),
      m_height(height),
      m_image(m_device, m_allocator),
      m_sampler(m_device) {
  if (m_device == nullptr || m_allocator == nullptr || m_ui == nullptr) {
    spdlog::error("VideoTexture: device, allocator or UI context is null.");
    throw std::runtime_error(
        "VideoTexture: device, allocator or UI context is null.");
  }
  if (width == 0 || height == 0) {
    spdlog::error("VideoTexture: size must be positive.");
    throw std::runtime_error("VideoTexture: size must be positive.");
  }
  ImageCreateInfo info;
  info.width = static_cast<int>(width);
  info.height = static_cast<int>(height);
  info.mipLevels = 1;
  info.numSample = VK_SAMPLE_COUNT_1_BIT;
  info.format = kFormat;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  info.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
  m_image.createImage(info);
  m_staging = Buffer(m_allocator, static_cast<VkDeviceSize>(width) * height * 4,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  createOverlayPass(framesInFlight);
  m_uiTexture = m_ui->registerTexture(m_sampler.get(), m_image.getImageView());
}

VideoTexture::~VideoTexture() { cleanUp(); }

void VideoTexture::cleanUp() {
  if (m_device == nullptr) {
    return;
  }
  m_device->waitIdle();
  if (m_uiTexture != VK_NULL_HANDLE) {
    m_ui->unregisterTexture(m_uiTexture);
    m_uiTexture = VK_NULL_HANDLE;
  }
  m_overlays.clear();
  m_overlayPipelines.reset();
  m_overlayCameraSets.clear();
  m_overlayDescriptorPool.reset();
  m_overlayCameraUbo.reset();
  m_overlayCameraLayout.reset();
  if (m_overlayFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(m_device->getDevice(), m_overlayFramebuffer, nullptr);
    m_overlayFramebuffer = VK_NULL_HANDLE;
  }
  if (m_overlayPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(m_device->getDevice(), m_overlayPass, nullptr);
    m_overlayPass = VK_NULL_HANDLE;
  }
  m_staging.cleanUp();
  m_sampler.cleanUp();
  m_image.cleanUp();
  m_device = nullptr;
}

void VideoTexture::createOverlayPass(uint32_t framesInFlight) {
  // Single colour attachment that keeps the uploaded frame (LOAD) and ends
  // up ready for sampling. No depth: overlays are flat.
  VkAttachmentDescription color = {};
  color.format = kFormat;
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  const VkAttachmentReference colorRef = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  // The copy must finish before the overlay reads/writes the attachment, and
  // the UI's fragment shader must wait for the pass to finish.
  const std::array<VkSubpassDependency, 2> dependencies = {
      VkSubpassDependency{
          .srcSubpass = VK_SUBPASS_EXTERNAL,
          .dstSubpass = 0,
          .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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
  passInfo.attachmentCount = 1;
  passInfo.pAttachments = &color;
  passInfo.subpassCount = 1;
  passInfo.pSubpasses = &subpass;
  passInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  passInfo.pDependencies = dependencies.data();
  VK_CHECK_RESULT(vkCreateRenderPass(m_device->getDevice(), &passInfo, nullptr,
                                     &m_overlayPass));

  const std::array<VkImageView, 1> attachments = {m_image.getImageView()};
  VkFramebufferCreateInfo fbInfo = {};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.renderPass = m_overlayPass;
  fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  fbInfo.pAttachments = attachments.data();
  fbInfo.width = m_width;
  fbInfo.height = m_height;
  fbInfo.layers = 1;
  VK_CHECK_RESULT(vkCreateFramebuffer(m_device->getDevice(), &fbInfo, nullptr,
                                      &m_overlayFramebuffer));

  m_overlayPipelines = std::make_unique<PipelineManager>(m_device);
  m_overlayCameraLayout = std::make_unique<DescriptorSetLayout>(
      m_device,
      std::vector<VkDescriptorSetLayoutBinding>{
          DescriptorSetLayout::uniformBuffer(
              0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)});
  m_overlayDescriptorPool = std::make_unique<DescriptorPool>(
      m_device,
      std::vector<VkDescriptorPoolSize>{
          {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           .descriptorCount = framesInFlight}},
      framesInFlight);
  m_overlayCameraUbo = std::make_unique<UniformBuffer>(
      m_allocator, sizeof(CameraUbo), framesInFlight);
  m_overlayCameraSets = m_overlayDescriptorPool->allocate(
      m_overlayCameraLayout->get(), framesInFlight);
  for (uint32_t frame = 0; frame < framesInFlight; ++frame) {
    m_overlayDescriptorPool->writeUniformBuffer(
        m_overlayCameraSets.at(frame), 0,
        m_overlayCameraUbo->descriptorInfo(frame));
    writeOverlayCamera(frame);
  }
}

void VideoTexture::writeOverlayCamera(uint32_t frameIndex) {
  // Pixel space: x right, y down (Vulkan clip space is y-down, so an ortho
  // with bottom = 0 and top = height maps y = 0 to the top edge).
  CameraUbo ubo;
  ubo.view = glm::mat4(1.0F);
  ubo.proj = glm::ortho(0.0F, static_cast<float>(m_width), 0.0F,
                        static_cast<float>(m_height), -1.0F, 1.0F);
  ubo.viewProj = ubo.proj;
  ubo.eye = glm::vec4(static_cast<float>(m_width) * 0.5F,
                      static_cast<float>(m_height) * 0.5F, 1.0F, 1.0F);
  m_overlayCameraUbo->write(frameIndex, ubo);
}

void VideoTexture::upload(std::span<const std::byte> rgba) {
  const size_t expected = static_cast<size_t>(m_width) * m_height * 4;
  if (rgba.size() != expected) {
    spdlog::error("VideoTexture: frame is {} bytes, expected {}.", rgba.size(),
                  expected);
    throw std::runtime_error("VideoTexture: frame has the wrong size.");
  }
  m_staging.write(rgba);
  m_hasFrame = true;
  ++m_uploads;
}

void VideoTexture::addOverlay(std::shared_ptr<IDrawable> drawable) {
  if (drawable == nullptr) {
    spdlog::error("VideoTexture::addOverlay: drawable is null.");
    throw std::runtime_error("VideoTexture::addOverlay: drawable is null.");
  }
  m_overlays.push_back(std::move(drawable));
}

bool VideoTexture::removeOverlay(const std::shared_ptr<IDrawable>& drawable) {
  auto it = std::ranges::find(m_overlays, drawable);
  if (it == m_overlays.end()) {
    return false;
  }
  m_device->waitIdle();
  m_overlays.erase(it);
  return true;
}

void VideoTexture::clearOverlays() {
  if (m_overlays.empty()) {
    return;
  }
  m_device->waitIdle();
  m_overlays.clear();
}

void VideoTexture::recordPrePass(VkCommandBuffer commandBuffer,
                                 uint32_t frameIndex) {
  if (!m_hasFrame) {
    return;
  }
  // 1. Whatever layout the image is in -> transfer destination.
  VkImageMemoryBarrier toTransfer = {};
  toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  toTransfer.oldLayout = m_layout;
  toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toTransfer.image = m_image.getImage();
  toTransfer.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel = 0,
                                 .levelCount = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1};
  toTransfer.srcAccessMask =
      m_layout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_SHADER_READ_BIT;
  toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  vkCmdPipelineBarrier(commandBuffer,
                       m_layout == VK_IMAGE_LAYOUT_UNDEFINED
                           ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                           : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &toTransfer);

  // 2. Staging buffer -> image.
  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .mipLevel = 0,
                             .baseArrayLayer = 0,
                             .layerCount = 1};
  region.imageOffset = {.x = 0, .y = 0, .z = 0};
  region.imageExtent = {.width = m_width, .height = m_height, .depth = 1};
  vkCmdCopyBufferToImage(commandBuffer, m_staging.get(), m_image.getImage(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  // 3. Overlay pass: TRANSFER_DST -> (draw) -> SHADER_READ_ONLY. The pass
  // itself performs both layout transitions, even with no overlays.
  VkRenderPassBeginInfo passBegin = {};
  passBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  passBegin.renderPass = m_overlayPass;
  passBegin.framebuffer = m_overlayFramebuffer;
  passBegin.renderArea.offset = {.x = 0, .y = 0};
  passBegin.renderArea.extent = {.width = m_width, .height = m_height};
  passBegin.clearValueCount = 0;
  vkCmdBeginRenderPass(commandBuffer, &passBegin, VK_SUBPASS_CONTENTS_INLINE);
  FrameContext context;
  context.commandBuffer = commandBuffer;
  context.frameIndex = frameIndex;
  context.extent = {.width = m_width, .height = m_height};
  context.renderPass = m_overlayPass;
  context.samples = VK_SAMPLE_COUNT_1_BIT;
  context.cameraSet = m_overlayCameraSets.at(frameIndex);
  context.cameraSetLayout = m_overlayCameraLayout->get();
  context.pipelines = m_overlayPipelines.get();
  for (const std::shared_ptr<IDrawable>& overlay : m_overlays) {
    overlay->record(context);
  }
  vkCmdEndRenderPass(commandBuffer);
  m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

}  // namespace avalon

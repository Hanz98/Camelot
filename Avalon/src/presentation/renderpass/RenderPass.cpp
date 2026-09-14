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

#include "RenderPass.h"

#include <Avalon/src/validation/CheckResult.h>
#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

RenderPass::RenderPass(std::shared_ptr<Device> device, VkFormat colorFormat,
                       VkFormat depthFormat, VkSampleCountFlagBits samples)
    : m_device(std::move(device)),
      m_renderPass(VK_NULL_HANDLE),
      m_colorFormat(colorFormat),
      m_depthFormat(depthFormat),
      m_samples(samples) {
  if (m_device == nullptr) {
    spdlog::error("RenderPass: Device is not initialized.");
    throw std::runtime_error("RenderPass: Device is not initialized.");
  }
  initialize();
}

RenderPass::~RenderPass() { cleanUp(); }

void RenderPass::cleanUp() {
  if (m_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;
  }
}

void RenderPass::initialize() {
  const bool msaa = isMultisampled();

  VkAttachmentDescription color = {};
  color.format = m_colorFormat;
  color.samples = m_samples;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp =
      msaa ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = msaa ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                           : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depth = {};
  depth.format = m_depthFormat;
  depth.samples = m_samples;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription resolve = {};
  resolve.format = m_colorFormat;
  resolve.samples = VK_SAMPLE_COUNT_1_BIT;
  resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  std::vector<VkAttachmentDescription> attachments = {color, depth};
  if (msaa) {
    attachments.push_back(resolve);
  }

  VkAttachmentReference colorRef = {0,
                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthRef = {
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkAttachmentReference resolveRef = {2,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;
  subpass.pResolveAttachments = msaa ? &resolveRef : nullptr;

  // Wait for the previous frame's colour/depth writes (and the presentation
  // engine's read of the swapchain image) before this pass clears them.
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = static_cast<uint32_t>(attachments.size());
  info.pAttachments = attachments.data();
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dependency;

  VK_CHECK_RESULT(
      vkCreateRenderPass(m_device->getDevice(), &info, nullptr, &m_renderPass));
}

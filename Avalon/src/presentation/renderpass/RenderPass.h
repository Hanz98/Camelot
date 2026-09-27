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

#ifndef AVALON_SRC_PRESENTATION_RENDERPASS_RENDERPASS_H_
#define AVALON_SRC_PRESENTATION_RENDERPASS_RENDERPASS_H_

#include <Avalon/src/device/Device.h>
#include <vulkan/vulkan.h>

#include <memory>

// A single-subpass render pass with a colour attachment, a depth attachment
// and, when multisampling is on, a resolve attachment onto the swapchain
// image. Attachment order (used by the framebuffers):
//   samples == 1:  [0] swapchain colour, [1] depth
//   samples  > 1:  [0] MSAA colour,      [1] depth, [2] swapchain colour
class RenderPass {
 private:
  std::shared_ptr<Device> m_device;
  VkRenderPass m_renderPass{VK_NULL_HANDLE};
  VkFormat m_colorFormat;
  VkFormat m_depthFormat;
  VkSampleCountFlagBits m_samples;

 public:
  RenderPass(std::shared_ptr<Device> device, VkFormat colorFormat,
             VkFormat depthFormat, VkSampleCountFlagBits samples);
  RenderPass(const RenderPass&) = delete;
  RenderPass& operator=(const RenderPass&) = delete;
  RenderPass(RenderPass&&) = delete;
  RenderPass& operator=(RenderPass&&) = delete;
  ~RenderPass();

  void cleanUp();

  [[nodiscard]] VkRenderPass getRenderPass() const { return m_renderPass; }
  [[nodiscard]] VkSampleCountFlagBits getSamples() const { return m_samples; }
  [[nodiscard]] bool isMultisampled() const {
    return m_samples != VK_SAMPLE_COUNT_1_BIT;
  }
  [[nodiscard]] VkFormat getColorFormat() const { return m_colorFormat; }
  [[nodiscard]] VkFormat getDepthFormat() const { return m_depthFormat; }

 private:
  void initialize();
};

#endif  // AVALON_SRC_PRESENTATION_RENDERPASS_RENDERPASS_H_

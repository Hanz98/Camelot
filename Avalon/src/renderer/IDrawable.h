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

#ifndef AVALON_SRC_RENDERER_IDRAWABLE_H_
#define AVALON_SRC_RENDERER_IDRAWABLE_H_

#include <vulkan/vulkan.h>

#include <cstdint>

#include "Avalon/src/pipeline/PipelineManager.h"

namespace avalon {

// Everything a drawable needs to record itself into the current frame's
// command buffer. Valid only during IDrawable::record().
struct FrameContext {
  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  uint32_t frameIndex{0};  // 0 .. framesInFlight - 1
  VkExtent2D extent{};
  VkRenderPass renderPass{VK_NULL_HANDLE};
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
  VkDescriptorSet cameraSet{VK_NULL_HANDLE};  // set 0: CameraUbo at binding 0
  VkDescriptorSetLayout cameraSetLayout{VK_NULL_HANDLE};
  PipelineManager* pipelines{nullptr};  // not owned
};

// Something the Renderer draws inside its render pass. Implementations own
// their GPU resources and fetch (or lazily build) their pipeline from
// FrameContext::pipelines.
class IDrawable {
 public:
  IDrawable() = default;
  IDrawable(const IDrawable&) = default;
  IDrawable& operator=(const IDrawable&) = default;
  IDrawable(IDrawable&&) = default;
  IDrawable& operator=(IDrawable&&) = default;
  virtual ~IDrawable() = default;

  virtual void record(const FrameContext& context) = 0;
};

}  // namespace avalon

#endif  // AVALON_SRC_RENDERER_IDRAWABLE_H_

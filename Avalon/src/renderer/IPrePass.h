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

#ifndef AVALON_SRC_RENDERER_IPREPASS_H_
#define AVALON_SRC_RENDERER_IPREPASS_H_

#include <vulkan/vulkan.h>

#include <cstdint>

namespace avalon {

// Work recorded into the frame's command buffer before the main render pass
// begins: texture uploads, offscreen passes that the main pass then samples.
class IPrePass {
 public:
  IPrePass() = default;
  IPrePass(const IPrePass&) = default;
  IPrePass& operator=(const IPrePass&) = default;
  IPrePass(IPrePass&&) = default;
  IPrePass& operator=(IPrePass&&) = default;
  virtual ~IPrePass() = default;

  virtual void recordPrePass(VkCommandBuffer commandBuffer,
                             uint32_t frameIndex) = 0;
};

}  // namespace avalon

#endif  // AVALON_SRC_RENDERER_IPREPASS_H_

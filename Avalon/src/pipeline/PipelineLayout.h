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

#ifndef AVALON_SRC_PIPELINE_PIPELINELAYOUT_H_
#define AVALON_SRC_PIPELINE_PIPELINELAYOUT_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

#include "Avalon/src/device/Device.h"

namespace avalon {

// RAII wrapper around VkPipelineLayout: the descriptor set layouts and push
// constant ranges a pipeline's shaders expect.
class PipelineLayout {
 private:
  std::shared_ptr<Device> m_device;
  VkPipelineLayout m_layout{VK_NULL_HANDLE};
  std::string m_name;

 public:
  // `name` is used in log and error messages only.
  PipelineLayout(std::shared_ptr<Device> device, std::string name,
                 const std::vector<VkDescriptorSetLayout>& setLayouts = {},
                 const std::vector<VkPushConstantRange>& pushConstants = {});
  PipelineLayout(const PipelineLayout&) = delete;
  PipelineLayout& operator=(const PipelineLayout&) = delete;
  PipelineLayout(PipelineLayout&& other) noexcept;
  PipelineLayout& operator=(PipelineLayout&& other) noexcept;
  ~PipelineLayout();

  void cleanUp();

  [[nodiscard]] VkPipelineLayout get() const { return m_layout; }
  [[nodiscard]] const std::string& name() const { return m_name; }
};

}  // namespace avalon

#endif  // AVALON_SRC_PIPELINE_PIPELINELAYOUT_H_

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

#ifndef AVALON_SRC_PIPELINE_PIPELINEMANAGER_H_
#define AVALON_SRC_PIPELINE_PIPELINEMANAGER_H_

#include <vulkan/vulkan.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Avalon/src/device/Device.h"
#include "Avalon/src/pipeline/GraphicsPipeline.h"
#include "Avalon/src/pipeline/PipelineLayout.h"

namespace avalon {

// Owns pipeline layouts and graphics pipelines and hands them out by name.
// A pipeline is built once and cached; clear() destroys everything (for
// example before the render pass changes).
class PipelineManager {
 public:
  // Builds a pipeline; receives the manager so it can fetch a layout. Shader
  // modules the factory creates only need to live until build() returns.
  using Factory = std::function<GraphicsPipeline(PipelineManager&)>;

 private:
  std::shared_ptr<Device> m_device;
  std::unordered_map<std::string, std::unique_ptr<PipelineLayout>> m_layouts;
  std::unordered_map<std::string, std::unique_ptr<GraphicsPipeline>>
      m_pipelines;

 public:
  explicit PipelineManager(std::shared_ptr<Device> device);
  PipelineManager(const PipelineManager&) = delete;
  PipelineManager& operator=(const PipelineManager&) = delete;
  PipelineManager(PipelineManager&&) = delete;
  PipelineManager& operator=(PipelineManager&&) = delete;
  ~PipelineManager();

  void clear();

  // Creates (or returns the existing) layout with this name.
  PipelineLayout& getOrCreateLayout(
      const std::string& name,
      const std::vector<VkDescriptorSetLayout>& setLayouts = {},
      const std::vector<VkPushConstantRange>& pushConstants = {});

  // Builds the pipeline through `factory` unless one with this name exists.
  GraphicsPipeline& getOrCreate(const std::string& name,
                                const Factory& factory);

  // The cached pipeline / layout; throws std::out_of_range naming it if
  // there is none.
  [[nodiscard]] GraphicsPipeline& get(const std::string& name) const;
  [[nodiscard]] PipelineLayout& getLayout(const std::string& name) const;
  [[nodiscard]] bool has(const std::string& name) const;
  [[nodiscard]] size_t size() const { return m_pipelines.size(); }

  // Destroys one cached pipeline (e.g. to rebuild it with new state).
  void remove(const std::string& name);
};

}  // namespace avalon

#endif  // AVALON_SRC_PIPELINE_PIPELINEMANAGER_H_

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

#ifndef AVALON_SRC_PIPELINE_GRAPHICSPIPELINE_H_
#define AVALON_SRC_PIPELINE_GRAPHICSPIPELINE_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

#include "Avalon/src/device/Device.h"
#include "Avalon/src/shader/ShaderModule.h"

namespace avalon {

class GraphicsPipelineBuilder;

// RAII wrapper around a graphics VkPipeline. Built through
// GraphicsPipelineBuilder; viewport and scissor are always dynamic state, so
// a pipeline survives swapchain recreation as long as the render pass
// formats do not change.
class GraphicsPipeline {
 private:
  std::shared_ptr<Device> m_device;
  VkPipeline m_pipeline{VK_NULL_HANDLE};
  VkPipelineLayout m_layout{VK_NULL_HANDLE};  // not owned
  VkPipelineBindPoint m_bindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS};
  std::string m_name;

  friend class GraphicsPipelineBuilder;
  GraphicsPipeline(std::shared_ptr<Device> device, VkPipeline pipeline,
                   VkPipelineLayout layout, std::string name);

 public:
  GraphicsPipeline(const GraphicsPipeline&) = delete;
  GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
  GraphicsPipeline(GraphicsPipeline&& other) noexcept;
  GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;
  ~GraphicsPipeline();

  void cleanUp();

  // Binds the pipeline and sets the viewport/scissor to `extent`.
  void bind(VkCommandBuffer commandBuffer, VkExtent2D extent) const;

  [[nodiscard]] VkPipeline get() const { return m_pipeline; }
  [[nodiscard]] VkPipelineLayout getLayout() const { return m_layout; }
  [[nodiscard]] const std::string& name() const { return m_name; }
};

// Fluent description of a graphics pipeline. Defaults: triangle list, fill,
// back-face culling off, depth test and write on, no blending, one sample.
// build() throws std::runtime_error naming the pipeline when the description
// is incomplete or the driver rejects it.
class GraphicsPipelineBuilder {
 private:
  std::string m_name;
  std::vector<VkPipelineShaderStageCreateInfo> m_stages;
  std::vector<VkVertexInputBindingDescription> m_bindings;
  std::vector<VkVertexInputAttributeDescription> m_attributes;
  VkPrimitiveTopology m_topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  VkPolygonMode m_polygonMode{VK_POLYGON_MODE_FILL};
  VkCullModeFlags m_cullMode{VK_CULL_MODE_NONE};
  VkFrontFace m_frontFace{VK_FRONT_FACE_COUNTER_CLOCKWISE};
  float m_lineWidth{1.0F};
  VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
  bool m_depthTest{true};
  bool m_depthWrite{true};
  bool m_blend{false};
  VkRenderPass m_renderPass{VK_NULL_HANDLE};
  uint32_t m_subpass{0};
  VkPipelineLayout m_layout{VK_NULL_HANDLE};

 public:
  explicit GraphicsPipelineBuilder(std::string name);

  GraphicsPipelineBuilder& addStage(const ShaderModule& module,
                                    const char* entryPoint = "main");
  GraphicsPipelineBuilder& addVertexBinding(
      uint32_t binding, uint32_t stride,
      VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX);
  GraphicsPipelineBuilder& addVertexAttribute(uint32_t location,
                                              uint32_t binding, VkFormat format,
                                              uint32_t offset);
  GraphicsPipelineBuilder& setTopology(VkPrimitiveTopology topology);
  GraphicsPipelineBuilder& setPolygonMode(VkPolygonMode mode);
  GraphicsPipelineBuilder& setCullMode(VkCullModeFlags mode,
                                       VkFrontFace frontFace);
  GraphicsPipelineBuilder& setLineWidth(float width);
  GraphicsPipelineBuilder& setSamples(VkSampleCountFlagBits samples);
  GraphicsPipelineBuilder& setDepth(bool test, bool write);
  GraphicsPipelineBuilder& setAlphaBlend(bool enable);
  GraphicsPipelineBuilder& setRenderPass(VkRenderPass renderPass,
                                         uint32_t subpass = 0);
  GraphicsPipelineBuilder& setLayout(VkPipelineLayout layout);

  [[nodiscard]] const std::string& name() const { return m_name; }

  [[nodiscard]] GraphicsPipeline build(
      const std::shared_ptr<Device>& device) const;
};

}  // namespace avalon

#endif  // AVALON_SRC_PIPELINE_GRAPHICSPIPELINE_H_

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

#include "Avalon/src/pipeline/GraphicsPipeline.h"

#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace avalon {

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device,
                                   VkPipeline pipeline, VkPipelineLayout layout,
                                   std::string name)
    : m_device(std::move(device)),
      m_pipeline(pipeline),
      m_layout(layout),
      m_name(std::move(name)) {}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_pipeline(std::exchange(other.m_pipeline, VK_NULL_HANDLE)),
      m_layout(std::exchange(other.m_layout, VK_NULL_HANDLE)),
      m_bindPoint(other.m_bindPoint),
      m_name(std::move(other.m_name)) {}

GraphicsPipeline& GraphicsPipeline::operator=(
    GraphicsPipeline&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = std::move(other.m_device);
    m_pipeline = std::exchange(other.m_pipeline, VK_NULL_HANDLE);
    m_layout = std::exchange(other.m_layout, VK_NULL_HANDLE);
    m_bindPoint = other.m_bindPoint;
    m_name = std::move(other.m_name);
  }
  return *this;
}

GraphicsPipeline::~GraphicsPipeline() { cleanUp(); }

void GraphicsPipeline::cleanUp() {
  if (m_pipeline != VK_NULL_HANDLE && m_device != nullptr) {
    vkDestroyPipeline(m_device->getDevice(), m_pipeline, nullptr);
  }
  m_pipeline = VK_NULL_HANDLE;
}

void GraphicsPipeline::bind(VkCommandBuffer commandBuffer,
                            VkExtent2D extent) const {
  vkCmdBindPipeline(commandBuffer, m_bindPoint, m_pipeline);
  VkViewport viewport = {};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  VkRect2D scissor = {};
  scissor.offset = {.x = 0, .y = 0};
  scissor.extent = extent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(std::string name)
    : m_name(std::move(name)) {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addStage(
    const ShaderModule& module, const char* entryPoint) {
  m_stages.push_back(module.stageInfo(entryPoint));
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexBinding(
    uint32_t binding, uint32_t stride, VkVertexInputRate rate) {
  m_bindings.push_back(
      {.binding = binding, .stride = stride, .inputRate = rate});
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute(
    uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) {
  m_attributes.push_back({.location = location,
                          .binding = binding,
                          .format = format,
                          .offset = offset});
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setTopology(
    VkPrimitiveTopology topology) {
  m_topology = topology;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setPolygonMode(
    VkPolygonMode mode) {
  m_polygonMode = mode;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setCullMode(
    VkCullModeFlags mode, VkFrontFace frontFace) {
  m_cullMode = mode;
  m_frontFace = frontFace;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setLineWidth(float width) {
  m_lineWidth = width;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setSamples(
    VkSampleCountFlagBits samples) {
  m_samples = samples;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepth(bool test,
                                                           bool write) {
  m_depthTest = test;
  m_depthWrite = write;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setAlphaBlend(bool enable) {
  m_blend = enable;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRenderPass(
    VkRenderPass renderPass, uint32_t subpass) {
  m_renderPass = renderPass;
  m_subpass = subpass;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setLayout(
    VkPipelineLayout layout) {
  m_layout = layout;
  return *this;
}

GraphicsPipeline GraphicsPipelineBuilder::build(
    const std::shared_ptr<Device>& device) const {
  auto fail = [this](const std::string& why) {
    const std::string message = "GraphicsPipeline '" + m_name + "': " + why;
    spdlog::error(message);
    throw std::runtime_error(message);
  };
  if (device == nullptr) {
    fail("device is not initialized.");
  }
  if (m_stages.empty()) {
    fail("no shader stages were added.");
  }
  if (m_renderPass == VK_NULL_HANDLE) {
    fail("no render pass was set.");
  }
  if (m_layout == VK_NULL_HANDLE) {
    fail("no pipeline layout was set.");
  }

  VkPipelineVertexInputStateCreateInfo vertexInput = {};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount =
      static_cast<uint32_t>(m_bindings.size());
  vertexInput.pVertexBindingDescriptions = m_bindings.data();
  vertexInput.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(m_attributes.size());
  vertexInput.pVertexAttributeDescriptions = m_attributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = m_topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport and scissor are dynamic; only the counts matter here.
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = m_polygonMode;
  rasterizer.cullMode = m_cullMode;
  rasterizer.frontFace = m_frontFace;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.lineWidth = m_lineWidth;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = m_samples;
  multisampling.sampleShadingEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = m_depthTest ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable = m_depthWrite ? VK_TRUE : VK_FALSE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState blendAttachment = {};
  blendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  blendAttachment.blendEnable = m_blend ? VK_TRUE : VK_FALSE;
  blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &blendAttachment;

  const std::array<VkDynamicState, 2> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkGraphicsPipelineCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.stageCount = static_cast<uint32_t>(m_stages.size());
  info.pStages = m_stages.data();
  info.pVertexInputState = &vertexInput;
  info.pInputAssemblyState = &inputAssembly;
  info.pViewportState = &viewportState;
  info.pRasterizationState = &rasterizer;
  info.pMultisampleState = &multisampling;
  info.pDepthStencilState = &depthStencil;
  info.pColorBlendState = &colorBlending;
  info.pDynamicState = &dynamicState;
  info.layout = m_layout;
  info.renderPass = m_renderPass;
  info.subpass = m_subpass;
  info.basePipelineHandle = VK_NULL_HANDLE;

  VkPipeline pipeline = VK_NULL_HANDLE;
  const VkResult result = vkCreateGraphicsPipelines(
      device->getDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
  if (result != VK_SUCCESS) {
    fail("vkCreateGraphicsPipelines failed with VkResult " +
         std::to_string(static_cast<int>(result)));
  }
  spdlog::debug("GraphicsPipeline '{}' created.", m_name);
  return {device, pipeline, m_layout, m_name};
}

}  // namespace avalon

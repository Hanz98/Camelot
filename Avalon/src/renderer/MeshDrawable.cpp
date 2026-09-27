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

#include "Avalon/src/renderer/MeshDrawable.h"

#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/pipeline/GraphicsPipeline.h"
#include "Avalon/src/shader/ShaderModule.h"

namespace avalon {

MeshDrawable::MeshDrawable(
    std::shared_ptr<Device> device,
    const std::shared_ptr<VmaAllocatorWrapper>& allocator, const MeshData& mesh)
    : m_device(std::move(device)) {
  if (mesh.empty()) {
    spdlog::error("MeshDrawable: mesh has no indices.");
    throw std::runtime_error("MeshDrawable: mesh has no indices.");
  }
  m_vertexBuffer = Buffer(allocator, mesh.vertices.size() * sizeof(Vertex),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  m_vertexBuffer.write(std::span<const Vertex>(mesh.vertices));
  m_indexBuffer = Buffer(allocator, mesh.indices.size() * sizeof(uint32_t),
                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  m_indexBuffer.write(std::span<const uint32_t>(mesh.indices));
  m_indexCount = static_cast<uint32_t>(mesh.indices.size());
}

GraphicsPipeline MeshDrawable::buildPipeline(
    const std::shared_ptr<Device>& device, PipelineManager& pipelines,
    const FrameContext& context) {
  const VkPushConstantRange pushRange = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = sizeof(MeshPushConstants)};
  const PipelineLayout& layout = pipelines.getOrCreateLayout(
      kPipeline, {context.cameraSetLayout}, {pushRange});
  const ShaderModule vert(device, shaders::get("mesh.vert"));
  const ShaderModule frag(device, shaders::get("mesh.frag"));
  GraphicsPipelineBuilder builder(kPipeline);
  builder.addStage(vert)
      .addStage(frag)
      .addVertexBinding(0, sizeof(Vertex))
      .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .setDepth(true, true)
      .setSamples(context.samples)
      .setRenderPass(context.renderPass)
      .setLayout(layout.get());
  for (const VkVertexInputAttributeDescription& attribute :
       Vertex::attributeDescriptions(0)) {
    builder.addVertexAttribute(attribute.location, attribute.binding,
                               attribute.format, attribute.offset);
  }
  return builder.build(device);
}

void MeshDrawable::record(const FrameContext& context) {
  if (!m_visible || context.pipelines == nullptr) {
    return;
  }
  const GraphicsPipeline& pipeline = context.pipelines->getOrCreate(
      kPipeline, [this, &context](PipelineManager& pm) {
        return buildPipeline(m_device, pm, context);
      });
  pipeline.bind(context.commandBuffer, context.extent);
  vkCmdBindDescriptorSets(context.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(),
                          0, 1, &context.cameraSet, 0, nullptr);
  vkCmdPushConstants(context.commandBuffer, pipeline.getLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(MeshPushConstants), &m_push);
  const std::array<VkBuffer, 1> vertexBuffers = {m_vertexBuffer.get()};
  const std::array<VkDeviceSize, 1> offsets = {0};
  vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, vertexBuffers.data(),
                         offsets.data());
  vkCmdBindIndexBuffer(context.commandBuffer, m_indexBuffer.get(), 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(context.commandBuffer, m_indexCount, 1, 0, 0, 0);
}

}  // namespace avalon

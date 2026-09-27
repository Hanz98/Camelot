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

#include "Avalon/src/renderer/PointCloudDrawable.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/pipeline/GraphicsPipeline.h"
#include "Avalon/src/shader/ShaderModule.h"

namespace avalon {

PointCloudDrawable::PointCloudDrawable(
    std::shared_ptr<Device> device,
    std::shared_ptr<VmaAllocatorWrapper> allocator,
    std::span<const PointVertex> points)
    : m_device(std::move(device)), m_allocator(std::move(allocator)) {
  if (m_device == nullptr || m_allocator == nullptr) {
    spdlog::error("PointCloudDrawable: device or allocator is null.");
    throw std::runtime_error(
        "PointCloudDrawable: device or allocator is null.");
  }
  setPoints(points);
}

void PointCloudDrawable::reserve(uint32_t points) {
  const uint32_t wanted = std::max(points, kMinCapacity);
  if (wanted <= m_capacity) {
    return;
  }
  // Grow geometrically so streams of growing clouds do not reallocate every
  // frame. The GPU may still read the old buffer, so wait before dropping it.
  uint32_t capacity = std::max(m_capacity, kMinCapacity);
  while (capacity < wanted) {
    capacity *= 2;
  }
  m_device->waitIdle();
  m_vertexBuffer = Buffer(m_allocator, capacity * sizeof(PointVertex),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  m_capacity = capacity;
}

void PointCloudDrawable::setPoints(std::span<const PointVertex> points) {
  reserve(static_cast<uint32_t>(points.size()));
  if (!points.empty()) {
    m_vertexBuffer.write(points);
  }
  m_count = static_cast<uint32_t>(points.size());
}

void PointCloudDrawable::setPoint(const glm::vec3& position,
                                  const glm::vec4& color) {
  const std::array<PointVertex, 1> point = {
      PointVertex{.position = position, .color = color}};
  setPoints(point);
}

GraphicsPipeline PointCloudDrawable::buildPipeline(
    const std::shared_ptr<Device>& device, PipelineManager& pipelines,
    const FrameContext& context) {
  const VkPushConstantRange pushRange = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = sizeof(PointPushConstants)};
  const PipelineLayout& layout = pipelines.getOrCreateLayout(
      kPipeline, {context.cameraSetLayout}, {pushRange});
  const ShaderModule vert(device, shaders::get("point.vert"));
  const ShaderModule frag(device, shaders::get("point.frag"));
  GraphicsPipelineBuilder builder(kPipeline);
  builder.addStage(vert)
      .addStage(frag)
      .addVertexBinding(0, sizeof(PointVertex))
      .setTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
      .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .setDepth(true, true)
      .setAlphaBlend(true)
      .setSamples(context.samples)
      .setRenderPass(context.renderPass)
      .setLayout(layout.get());
  for (const VkVertexInputAttributeDescription& attribute :
       PointVertex::attributeDescriptions(0)) {
    builder.addVertexAttribute(attribute.location, attribute.binding,
                               attribute.format, attribute.offset);
  }
  return builder.build(device);
}

void PointCloudDrawable::record(const FrameContext& context) {
  if (!m_visible || m_count == 0 || context.pipelines == nullptr) {
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
                     0, sizeof(PointPushConstants), &m_push);
  const std::array<VkBuffer, 1> vertexBuffers = {m_vertexBuffer.get()};
  const std::array<VkDeviceSize, 1> offsets = {0};
  vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, vertexBuffers.data(),
                         offsets.data());
  vkCmdDraw(context.commandBuffer, m_count, 1, 0, 0);
}

}  // namespace avalon

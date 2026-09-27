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

#include "Avalon/src/renderer/LineDrawable.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/pipeline/GraphicsPipeline.h"
#include "Avalon/src/shader/ShaderModule.h"

namespace avalon {

LineDrawable::LineDrawable(std::shared_ptr<Device> device,
                           std::shared_ptr<VmaAllocatorWrapper> allocator,
                           std::span<const PointVertex> lines)
    : m_device(std::move(device)),
      m_allocator(std::move(allocator)),
      m_pipelineName(kPipeline) {
  if (m_device == nullptr || m_allocator == nullptr) {
    spdlog::error("LineDrawable: device or allocator is null.");
    throw std::runtime_error("LineDrawable: device or allocator is null.");
  }
  setLines(lines);
}

void LineDrawable::reserve(uint32_t vertices) {
  const uint32_t wanted = std::max(vertices, kMinCapacity);
  if (wanted <= m_capacity) {
    return;
  }
  // Grow geometrically so streams of growing line sets do not reallocate
  // every frame. The GPU may still read the old buffer, so wait before
  // dropping it.
  uint32_t capacity = std::max(m_capacity, kMinCapacity);
  while (capacity < wanted) {
    capacity *= 2;
  }
  m_device->waitIdle();
  m_vertexBuffer = Buffer(m_allocator, capacity * sizeof(PointVertex),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  m_capacity = capacity;
}

void LineDrawable::setLines(std::span<const PointVertex> lines) {
  if (lines.size() % 2 != 0) {
    spdlog::error("LineDrawable: {} vertices do not form line pairs.",
                  lines.size());
    throw std::runtime_error(
        "LineDrawable: odd vertex count, lines are vertex pairs.");
  }
  reserve(static_cast<uint32_t>(lines.size()));
  if (!lines.empty()) {
    m_vertexBuffer.write(lines);
  }
  m_count = static_cast<uint32_t>(lines.size());
}

void LineDrawable::setStrip(std::span<const PointVertex> points, bool closed) {
  if (points.size() < 2) {
    setLines({});
    return;
  }
  const bool loop = closed && points.size() >= 3;
  const size_t segments = points.size() - 1 + (loop ? 1 : 0);
  std::vector<PointVertex> pairs;
  pairs.reserve(segments * 2);
  for (size_t i = 0; i + 1 < points.size(); ++i) {
    pairs.push_back(points[i]);
    pairs.push_back(points[i + 1]);
  }
  if (loop) {
    pairs.push_back(points.back());
    pairs.push_back(points.front());
  }
  setLines(pairs);
}

float LineDrawable::clampLineWidth(Device& device, float lineWidth) {
  if (!std::isfinite(lineWidth)) {
    return kDefaultLineWidth;
  }
  const vkb::PhysicalDevice& physical = device.getVkbPhysicalDevice();
  // `features` holds the features enabled on the logical device (Device
  // asks for wideLines when present); without it only 1.0 is legal.
  if (physical.features.wideLines != VK_TRUE) {
    return kDefaultLineWidth;
  }
  const VkPhysicalDeviceLimits& limits = physical.properties.limits;
  float width = lineWidth;
  // The driver only supports widths on the granularity grid; snapping here
  // keeps equal requests on one pipeline variant.
  if (limits.lineWidthGranularity > 0.0F) {
    width = std::round(width / limits.lineWidthGranularity) *
            limits.lineWidthGranularity;
  }
  return std::clamp(width, limits.lineWidthRange[0], limits.lineWidthRange[1]);
}

std::string LineDrawable::pipelineNameFor(float lineWidth) {
  if (lineWidth == kDefaultLineWidth) {
    return kPipeline;
  }
  return fmt::format("{}@{:g}", kPipeline, lineWidth);
}

void LineDrawable::setLineWidth(float width) {
  m_lineWidth = clampLineWidth(*m_device, width);
  m_pipelineName = pipelineNameFor(m_lineWidth);
}

GraphicsPipeline LineDrawable::buildPipeline(
    const std::shared_ptr<Device>& device, PipelineManager& pipelines,
    const FrameContext& context, float lineWidth) {
  const VkPushConstantRange pushRange = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = sizeof(LinePushConstants)};
  // One layout for every width variant.
  const PipelineLayout& layout = pipelines.getOrCreateLayout(
      kPipeline, {context.cameraSetLayout}, {pushRange});
  const ShaderModule vert(device, shaders::get("line.vert"));
  const ShaderModule frag(device, shaders::get("line.frag"));
  GraphicsPipelineBuilder builder(pipelineNameFor(lineWidth));
  builder.addStage(vert)
      .addStage(frag)
      .addVertexBinding(0, sizeof(PointVertex))
      .setTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
      .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .setLineWidth(lineWidth)
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

void LineDrawable::record(const FrameContext& context) {
  if (!m_visible || m_count == 0 || context.pipelines == nullptr) {
    return;
  }
  const GraphicsPipeline& pipeline = context.pipelines->getOrCreate(
      m_pipelineName, [this, &context](PipelineManager& pm) {
        return buildPipeline(m_device, pm, context, m_lineWidth);
      });
  pipeline.bind(context.commandBuffer, context.extent);
  vkCmdBindDescriptorSets(context.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(),
                          0, 1, &context.cameraSet, 0, nullptr);
  vkCmdPushConstants(context.commandBuffer, pipeline.getLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(LinePushConstants), &m_push);
  const std::array<VkBuffer, 1> vertexBuffers = {m_vertexBuffer.get()};
  const std::array<VkDeviceSize, 1> offsets = {0};
  vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, vertexBuffers.data(),
                         offsets.data());
  vkCmdDraw(context.commandBuffer, m_count, 1, 0, 0);
}

}  // namespace avalon

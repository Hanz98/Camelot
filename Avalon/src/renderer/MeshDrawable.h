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

#ifndef AVALON_SRC_RENDERER_MESHDRAWABLE_H_
#define AVALON_SRC_RENDERER_MESHDRAWABLE_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/data/buffers/Buffer.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/geometry/Vertex.h"
#include "Avalon/src/renderer/IDrawable.h"

namespace avalon {

// Per-draw data of the mesh pipeline (push constants, 80 bytes).
struct MeshPushConstants {
  glm::mat4 model{1.0F};
  glm::vec4 color{1.0F};
};
static_assert(sizeof(MeshPushConstants) == 80);

// An indexed triangle mesh drawn with the lit `mesh` pipeline. The vertex
// and index data are uploaded once at construction; the transform and tint
// are push constants and can change every frame.
class MeshDrawable : public IDrawable {
 private:
  std::shared_ptr<Device> m_device;
  Buffer m_vertexBuffer;
  Buffer m_indexBuffer;
  uint32_t m_indexCount{0};
  MeshPushConstants m_push;
  bool m_visible{true};

 public:
  static constexpr const char* kPipeline = "mesh";

  // Throws std::runtime_error if `mesh` has no indices.
  MeshDrawable(std::shared_ptr<Device> device,
               const std::shared_ptr<VmaAllocatorWrapper>& allocator,
               const MeshData& mesh);

  void setTransform(const glm::mat4& model) { m_push.model = model; }
  [[nodiscard]] const glm::mat4& getTransform() const { return m_push.model; }
  void setColor(const glm::vec4& color) { m_push.color = color; }
  [[nodiscard]] const glm::vec4& getColor() const { return m_push.color; }
  void setVisible(bool visible) { m_visible = visible; }
  [[nodiscard]] bool isVisible() const { return m_visible; }
  [[nodiscard]] uint32_t indexCount() const { return m_indexCount; }

  void record(const FrameContext& context) override;

  // Builds the shared `mesh` pipeline for the render pass in `context`.
  static GraphicsPipeline buildPipeline(const std::shared_ptr<Device>& device,
                                        PipelineManager& pipelines,
                                        const FrameContext& context);
};

}  // namespace avalon

#endif  // AVALON_SRC_RENDERER_MESHDRAWABLE_H_

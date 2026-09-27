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

#ifndef AVALON_SRC_RENDERER_POINTCLOUDDRAWABLE_H_
#define AVALON_SRC_RENDERER_POINTCLOUDDRAWABLE_H_

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include <glm/glm.hpp>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/data/buffers/Buffer.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/geometry/Vertex.h"
#include "Avalon/src/renderer/IDrawable.h"

namespace avalon {

// Per-draw data of the point pipeline (push constants, 96 bytes).
struct PointPushConstants {
  glm::mat4 model{1.0F};
  glm::vec4 tint{1.0F};
  float pointSize{4.0F};
  std::array<float, 3> padding{};
};
static_assert(sizeof(PointPushConstants) == 96);

// A set of points drawn as round sprites with the `points` pipeline. One
// point is the smallest case; setPoints() replaces the whole set and grows
// the vertex buffer as needed. Points live in a host-visible buffer for now;
// roadmap T5 adds a staged upload for clouds in the millions.
class PointCloudDrawable : public IDrawable {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  Buffer m_vertexBuffer;
  uint32_t m_capacity{0};
  uint32_t m_count{0};
  PointPushConstants m_push;
  bool m_visible{true};

 public:
  static constexpr const char* kPipeline = "points";
  static constexpr uint32_t kMinCapacity = 64;

  PointCloudDrawable(std::shared_ptr<Device> device,
                     std::shared_ptr<VmaAllocatorWrapper> allocator,
                     std::span<const PointVertex> points = {});

  // Replaces every point. An empty span draws nothing but keeps the buffer.
  void setPoints(std::span<const PointVertex> points);
  // Convenience for the single-point case.
  void setPoint(const glm::vec3& position, const glm::vec4& color);

  void setPointSize(float pixels) { m_push.pointSize = pixels; }
  [[nodiscard]] float getPointSize() const { return m_push.pointSize; }
  void setTransform(const glm::mat4& model) { m_push.model = model; }
  [[nodiscard]] const glm::mat4& getTransform() const { return m_push.model; }
  void setTint(const glm::vec4& tint) { m_push.tint = tint; }
  [[nodiscard]] const glm::vec4& getTint() const { return m_push.tint; }
  void setVisible(bool visible) { m_visible = visible; }
  [[nodiscard]] bool isVisible() const { return m_visible; }
  [[nodiscard]] uint32_t pointCount() const { return m_count; }
  [[nodiscard]] uint32_t capacity() const { return m_capacity; }

  void record(const FrameContext& context) override;

  static GraphicsPipeline buildPipeline(const std::shared_ptr<Device>& device,
                                        PipelineManager& pipelines,
                                        const FrameContext& context);

 private:
  void reserve(uint32_t points);
};

}  // namespace avalon

#endif  // AVALON_SRC_RENDERER_POINTCLOUDDRAWABLE_H_

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

#ifndef AVALON_SRC_RENDERER_LINEDRAWABLE_H_
#define AVALON_SRC_RENDERER_LINEDRAWABLE_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include <glm/glm.hpp>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/data/buffers/Buffer.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/geometry/Vertex.h"
#include "Avalon/src/renderer/IDrawable.h"

namespace avalon {

// Per-draw data of the line pipeline (push constants, 80 bytes). Same layout
// as MeshPushConstants: model matrix followed by an RGBA tint.
struct LinePushConstants {
  glm::mat4 model{1.0F};
  glm::vec4 tint{1.0F};
};
static_assert(sizeof(LinePushConstants) == 80);

// Line segments drawn with the `lines` pipeline (VK_PRIMITIVE_TOPOLOGY_
// LINE_LIST): every two vertices form one segment. setLines() replaces the
// whole set and grows the vertex buffer geometrically like
// PointCloudDrawable; setStrip() expands a polyline into pairs on the CPU.
// The pipeline reads the camera UBO at set 0, so the same drawable works in
// the main render pass, in SceneViewWidget targets and as a VideoTexture
// overlay in pixel space. The tint multiplies the vertex colours; a tint
// alpha below one blends the lines (wireframes over solid meshes).
//
// Line width: the pipeline builder has no dynamic line-width state, so the
// width is baked into the pipeline. Width 1.0 uses the shared `lines`
// pipeline; other widths get their own cached variant named
// `lines@<width>` (see pipelineName()). Widths are clamped to the device's
// lineWidthRange and rounded to its lineWidthGranularity, which keeps the
// number of variants bounded; without the wideLines feature the width is
// always 1.0.
class LineDrawable : public IDrawable {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  Buffer m_vertexBuffer;
  uint32_t m_capacity{0};
  uint32_t m_count{0};
  LinePushConstants m_push;
  float m_lineWidth{1.0F};
  std::string m_pipelineName;
  bool m_visible{true};

 public:
  static constexpr const char* kPipeline = "lines";
  static constexpr uint32_t kMinCapacity = 64;
  static constexpr float kDefaultLineWidth = 1.0F;

  // Throws std::runtime_error when device or allocator is null or `lines`
  // has an odd number of vertices.
  LineDrawable(std::shared_ptr<Device> device,
               std::shared_ptr<VmaAllocatorWrapper> allocator,
               std::span<const PointVertex> lines = {});

  // Replaces every segment; `lines` holds vertex pairs (LINE_LIST). An odd
  // count throws std::runtime_error and leaves the drawable unchanged. An
  // empty span draws nothing but keeps the buffer.
  void setLines(std::span<const PointVertex> lines);
  // Replaces every segment with the polyline through `points`; `closed`
  // adds the segment from the last point back to the first when the strip
  // has at least three points. Fewer than two points draw nothing.
  void setStrip(std::span<const PointVertex> points, bool closed = false);

  void setTransform(const glm::mat4& model) { m_push.model = model; }
  [[nodiscard]] const glm::mat4& getTransform() const { return m_push.model; }
  void setTint(const glm::vec4& tint) { m_push.tint = tint; }
  [[nodiscard]] const glm::vec4& getTint() const { return m_push.tint; }
  void setVisible(bool visible) { m_visible = visible; }
  [[nodiscard]] bool isVisible() const { return m_visible; }
  // Clamps to the device's lineWidthRange (1.0 without wideLines); the
  // getter returns the width that is actually used.
  void setLineWidth(float width);
  [[nodiscard]] float getLineWidth() const { return m_lineWidth; }
  // Name of the pipeline variant this drawable binds (`lines` at width 1).
  [[nodiscard]] const std::string& pipelineName() const {
    return m_pipelineName;
  }
  [[nodiscard]] uint32_t vertexCount() const { return m_count; }
  [[nodiscard]] uint32_t segmentCount() const { return m_count / 2; }
  [[nodiscard]] uint32_t capacity() const { return m_capacity; }

  void record(const FrameContext& context) override;

  // Builds the `lines` pipeline (or the variant for `lineWidth`) for the
  // render pass in `context`. The pipeline layout is shared by every width.
  static GraphicsPipeline buildPipeline(const std::shared_ptr<Device>& device,
                                        PipelineManager& pipelines,
                                        const FrameContext& context,
                                        float lineWidth = kDefaultLineWidth);

  // Width `lineWidth` after applying the device's limits, as setLineWidth()
  // does.
  static float clampLineWidth(Device& device, float lineWidth);
  // Pipeline name for a (clamped) width: kPipeline for 1.0, `lines@<w>`
  // otherwise.
  static std::string pipelineNameFor(float lineWidth);

 private:
  void reserve(uint32_t vertices);
};

}  // namespace avalon

#endif  // AVALON_SRC_RENDERER_LINEDRAWABLE_H_

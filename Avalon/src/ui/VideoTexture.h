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

#ifndef AVALON_SRC_UI_VIDEOTEXTURE_H_
#define AVALON_SRC_UI_VIDEOTEXTURE_H_

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/data/buffers/Buffer.h"
#include "Avalon/src/data/buffers/UniformBuffer.h"
#include "Avalon/src/descriptor/DescriptorPool.h"
#include "Avalon/src/descriptor/DescriptorSetLayout.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/pipeline/PipelineManager.h"
#include "Avalon/src/presentation/image/Image.h"
#include "Avalon/src/presentation/image/Sampler.h"
#include "Avalon/src/renderer/IDrawable.h"
#include "Avalon/src/renderer/IPrePass.h"
#include "Avalon/src/ui/UiContext.h"

namespace avalon {

// An RGBA8 texture the UI can show with ImGui::Image(), fed from the CPU with
// upload(), plus a GPU overlay: drawables added with addOverlay() are drawn
// onto the frame in pixel coordinates (x right, y down, origin top-left)
// before the UI samples it.
//
// As a pre-pass (register it with Renderer::addPrePass) it records, every
// frame: staging buffer -> image copy, then the overlay render pass, leaving
// the image in SHADER_READ_ONLY_OPTIMAL.
class VideoTexture : public IPrePass {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  const UiContext* m_ui;  // not owned
  uint32_t m_width;
  uint32_t m_height;
  Image m_image;
  Sampler m_sampler;
  Buffer m_staging;
  VkDescriptorSet m_uiTexture{VK_NULL_HANDLE};
  VkImageLayout m_layout{VK_IMAGE_LAYOUT_UNDEFINED};
  bool m_hasFrame{false};
  uint64_t m_uploads{0};

  // Overlay pass resources.
  VkRenderPass m_overlayPass{VK_NULL_HANDLE};
  VkFramebuffer m_overlayFramebuffer{VK_NULL_HANDLE};
  std::unique_ptr<PipelineManager> m_overlayPipelines;
  std::unique_ptr<DescriptorSetLayout> m_overlayCameraLayout;
  std::unique_ptr<DescriptorPool> m_overlayDescriptorPool;
  std::unique_ptr<UniformBuffer> m_overlayCameraUbo;
  std::vector<VkDescriptorSet> m_overlayCameraSets;
  std::vector<std::shared_ptr<IDrawable>> m_overlays;

 public:
  static constexpr VkFormat kFormat = VK_FORMAT_R8G8B8A8_UNORM;

  VideoTexture(std::shared_ptr<Device> device,
               std::shared_ptr<VmaAllocatorWrapper> allocator,
               const UiContext* ui, uint32_t width, uint32_t height,
               uint32_t framesInFlight);
  VideoTexture(const VideoTexture&) = delete;
  VideoTexture& operator=(const VideoTexture&) = delete;
  VideoTexture(VideoTexture&&) = delete;
  VideoTexture& operator=(VideoTexture&&) = delete;
  ~VideoTexture() override;

  void cleanUp();

  // Copies one RGBA8 frame (width * height * 4 bytes) into the staging
  // buffer; it reaches the GPU in the next recordPrePass().
  void upload(std::span<const std::byte> rgba);

  void addOverlay(std::shared_ptr<IDrawable> drawable);
  bool removeOverlay(const std::shared_ptr<IDrawable>& drawable);
  void clearOverlays();
  [[nodiscard]] size_t overlayCount() const { return m_overlays.size(); }

  void recordPrePass(VkCommandBuffer commandBuffer,
                     uint32_t frameIndex) override;

  // The ImGui texture handle for ImGui::Image().
  [[nodiscard]] VkDescriptorSet uiTexture() const { return m_uiTexture; }
  [[nodiscard]] uint32_t width() const { return m_width; }
  [[nodiscard]] uint32_t height() const { return m_height; }
  [[nodiscard]] bool hasFrame() const { return m_hasFrame; }
  [[nodiscard]] uint64_t uploadCount() const { return m_uploads; }
  [[nodiscard]] VkImageLayout currentLayout() const { return m_layout; }
  [[nodiscard]] VkRenderPass overlayRenderPass() const { return m_overlayPass; }

 private:
  void createOverlayPass(uint32_t framesInFlight);
  void writeOverlayCamera(uint32_t frameIndex);
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_VIDEOTEXTURE_H_

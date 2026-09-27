#ifndef AVALON_SRC_UI_RENDERTARGET_H_
#define AVALON_SRC_UI_RENDERTARGET_H_

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/presentation/image/Image.h"
#include "Avalon/src/presentation/image/Sampler.h"
#include "Avalon/src/ui/UiContext.h"

namespace avalon {

// An offscreen colour + depth target the UI can show with ImGui::Image().
// The render pass clears both attachments and leaves the colour image in
// SHADER_READ_ONLY_OPTIMAL. resize() rebuilds everything for a new size
// (waits for the device first) and re-registers the UI texture.
class RenderTarget {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  const UiContext* m_ui;  // not owned
  uint32_t m_width{0};
  uint32_t m_height{0};
  VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};
  Image m_color;
  Image m_depth;
  Sampler m_sampler;
  VkRenderPass m_renderPass{VK_NULL_HANDLE};
  VkFramebuffer m_framebuffer{VK_NULL_HANDLE};
  VkDescriptorSet m_uiTexture{VK_NULL_HANDLE};
  uint64_t m_generation{0};

 public:
  static constexpr VkFormat kColorFormat = VK_FORMAT_R8G8B8A8_UNORM;

  RenderTarget(std::shared_ptr<Device> device,
               std::shared_ptr<VmaAllocatorWrapper> allocator,
               const UiContext* ui, uint32_t width, uint32_t height);
  RenderTarget(const RenderTarget&) = delete;
  RenderTarget& operator=(const RenderTarget&) = delete;
  RenderTarget(RenderTarget&&) = delete;
  RenderTarget& operator=(RenderTarget&&) = delete;
  ~RenderTarget();

  void cleanUp();

  // Rebuilds the images for a new size; no-op if unchanged or zero.
  // Returns true if the target was rebuilt.
  bool resize(uint32_t width, uint32_t height);

  void beginPass(VkCommandBuffer commandBuffer,
                 const std::array<float, 4>& clearColor) const;
  void endPass(VkCommandBuffer commandBuffer) const;

  [[nodiscard]] VkRenderPass renderPass() const { return m_renderPass; }
  [[nodiscard]] VkExtent2D extent() const {
    return {.width = m_width, .height = m_height};
  }
  [[nodiscard]] uint32_t width() const { return m_width; }
  [[nodiscard]] uint32_t height() const { return m_height; }
  [[nodiscard]] VkDescriptorSet uiTexture() const { return m_uiTexture; }
  // Incremented on every rebuild, so users can drop cached state.
  [[nodiscard]] uint64_t generation() const { return m_generation; }

 private:
  void create(uint32_t width, uint32_t height);
  void destroyImages();
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_RENDERTARGET_H_

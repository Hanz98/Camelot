#ifndef AVALON_SRC_UI_SCENEVIEWWIDGET_H_
#define AVALON_SRC_UI_SCENEVIEWWIDGET_H_

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/camera/Camera.h"
#include "Avalon/src/camera/CameraController.h"
#include "Avalon/src/data/buffers/UniformBuffer.h"
#include "Avalon/src/descriptor/DescriptorPool.h"
#include "Avalon/src/descriptor/DescriptorSetLayout.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/pipeline/PipelineManager.h"
#include "Avalon/src/renderer/IDrawable.h"
#include "Avalon/src/renderer/IPrePass.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/RenderTarget.h"
#include "Avalon/src/ui/UiContext.h"

namespace avalon {

// The 3D scene as a widget: renders the renderer's drawables into its own
// offscreen target (as a pre-pass) with its own camera, and shows the
// result in the UI. Dragging inside the view orbits (left), pans (right or
// middle) and the wheel zooms. Several views can show the same scene from
// different cameras; the target follows the window size.
class SceneViewWidget : public IPrePass {
 private:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  const UiContext* m_ui;      // not owned
  const Renderer* m_renderer;  // not owned; source of the drawables
  std::string m_title;
  std::unique_ptr<RenderTarget> m_target;
  Camera m_camera;
  CameraController m_controller;
  std::unique_ptr<PipelineManager> m_pipelines;
  std::unique_ptr<DescriptorSetLayout> m_cameraLayout;
  std::unique_ptr<DescriptorPool> m_descriptorPool;
  std::unique_ptr<UniformBuffer> m_cameraUbo;
  std::vector<VkDescriptorSet> m_cameraSets;
  std::array<float, 4> m_clearColor{0.05F, 0.05F, 0.08F, 1.0F};
  uint32_t m_requestedWidth{0};
  uint32_t m_requestedHeight{0};
  uint64_t m_renderedFrames{0};
  bool m_hovered{false};

 public:
  static constexpr uint32_t kMinSize = 16;

  SceneViewWidget(std::string title, std::shared_ptr<Device> device,
                  std::shared_ptr<VmaAllocatorWrapper> allocator,
                  const UiContext* ui, const Renderer* renderer,
                  uint32_t framesInFlight, uint32_t width = 640,
                  uint32_t height = 480);

  // Renders the scene into the target (resizing it first if the UI asked).
  void recordPrePass(VkCommandBuffer commandBuffer,
                     uint32_t frameIndex) override;

  // ImGui: the view image filling the window plus the camera controls.
  void draw();
  // Asks for a new target size; applied at the next recordPrePass().
  void requestSize(uint32_t width, uint32_t height);

  [[nodiscard]] Camera& camera() { return m_camera; }
  [[nodiscard]] const Camera& camera() const { return m_camera; }
  [[nodiscard]] CameraController& controller() { return m_controller; }
  [[nodiscard]] const RenderTarget& target() const { return *m_target; }
  [[nodiscard]] const std::string& title() const { return m_title; }
  [[nodiscard]] uint64_t renderedFrames() const { return m_renderedFrames; }
  [[nodiscard]] bool isHovered() const { return m_hovered; }
  void setClearColor(const std::array<float, 4>& color) {
    m_clearColor = color;
  }

 private:
  void writeCamera(uint32_t frameIndex);
  void handleInput();
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_SCENEVIEWWIDGET_H_

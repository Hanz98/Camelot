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

#ifndef AVALON_SRC_MAIN_AVALON_H_
#define AVALON_SRC_MAIN_AVALON_H_
#include <chrono>
#include <functional>
#include <memory>
#include <utility>

#include "Avalon/pch.h"
#include "Avalon/src/allocator/VmaAllocator.h"
#include "Avalon/src/camera/CameraController.h"
#include "Avalon/src/device/Device.h"
#include "Avalon/src/device/Instance.h"
#include "Avalon/src/presentation/renderpass/RenderPass.h"
#include "Avalon/src/presentation/swapchain/SwapchainModel.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/ui/UiContext.h"
#include "Avalon/src/window/SurfaceManager.h"
#include "Avalon/src/window/Window.h"

namespace avalon {

class Avalon {
 private:
  // Declaration order matters: members are destroyed in reverse order, so
  // the window (GLFW) outlives the instance, which outlives the surfaces,
  // which outlive the device, which outlives the allocator and swapchain.
  std::shared_ptr<Window> m_window;
  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<SurfaceManager> m_surfaceManager;
  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  std::shared_ptr<SwapchainModel> m_swapchainModel;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<CameraController> m_cameraController;
  std::unique_ptr<UiContext> m_ui;
  std::function<void()> m_uiCallback;
  std::chrono::steady_clock::time_point m_lastFrameStart;
  double m_lastFrameSeconds{0.0};

 public:
  Avalon();
  Avalon(const Avalon& other) = delete;
  Avalon(Avalon&& other) = delete;
  Avalon& operator=(const Avalon& other) = delete;
  Avalon& operator=(Avalon&& other) = delete;

  ~Avalon();
  void cleanUp();

  void init();
  [[nodiscard]] bool isInitialized() const;

  void test();

  // One iteration of the frame loop: process window events and draw.
  // Returns false once the window has been asked to close.
  bool frame();
  [[nodiscard]] bool shouldClose() const;
  [[nodiscard]] Renderer* getRenderer() const { return m_renderer.get(); }

  [[nodiscard]] std::shared_ptr<Window> getWindow() const { return m_window; }
  [[nodiscard]] std::shared_ptr<Device> getDevice() const { return m_device; }
  [[nodiscard]] std::shared_ptr<VmaAllocatorWrapper> getAllocator() const {
    return m_allocator;
  }
  // The renderer's camera and the mouse controller attached to the window.
  [[nodiscard]] Camera* getCamera() const {
    return m_renderer ? &m_renderer->getCamera() : nullptr;
  }
  [[nodiscard]] CameraController* getCameraController() const {
    return m_cameraController.get();
  }
  [[nodiscard]] UiContext* getUi() const { return m_ui.get(); }

  // Called once per frame between UiContext::newFrame() and rendering; build
  // the ImGui widgets here.
  void setUiCallback(std::function<void()> callback) {
    m_uiCallback = std::move(callback);
  }
  // Wall-clock duration of the previous frame() call, 0 before the first.
  [[nodiscard]] double getLastFrameSeconds() const {
    return m_lastFrameSeconds;
  }

 private:
  void initVma();
};

}  // namespace avalon

#endif  // AVALON_SRC_MAIN_AVALON_H_

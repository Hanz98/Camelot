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
#include <Avalon/src/allocator/VmaAllocator.h>
#include <Avalon/src/device/Device.h>
#include <Avalon/src/device/Instance.h>
#include <Avalon/src/presentation/renderpass/RenderPass.h>
#include <Avalon/src/presentation/swapchain/SwapChainModel.h>
#include <Avalon/src/window/SurfaceManager.h>
#include <Avalon/src/window/Window.h>
#include <pch.h>

#include <memory>

class Avalon {
 private:
  std::shared_ptr<SurfaceManager> m_surfaceManager;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;
  std::shared_ptr<SwapchainModel> m_swapchainModel;

 public:
  Avalon();
  Avalon(const Avalon& other) = delete;
  Avalon(Avalon&& other) = delete;
  Avalon& operator=(const Avalon& other) = delete;
  Avalon& operator=(Avalon&& other) = delete;

  ~Avalon();
  void cleanUp();

  void init();

  void test();

 private:
  void initVma();
};

#endif  // AVALON_SRC_MAIN_AVALON_H_

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

#ifndef AVALON_SRC_PRESENTATION_SWAPCHAIN_SWAPCHAINMODEL_H_
#define AVALON_SRC_PRESENTATION_SWAPCHAIN_SWAPCHAINMODEL_H_

#include <Avalon/src/device/Device.h>
#include <Avalon/src/presentation/image/Image.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

class SwapchainModel {
 public:
  std::shared_ptr<Device> m_device;
  std::shared_ptr<Window> m_window;

  Image m_depth;
  Image m_color;

  std::vector<Image> m_swapChainImage;
  std::vector<VkFrameBuffer> m_swapChainImage;

 private:
  SwapchainModel();
};

#endif  // AVALON_SRC_PRESENTATION_SWAPCHAIN_SWAPCHAINMODEL_H_

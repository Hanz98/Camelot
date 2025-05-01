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

#ifndef AVALON_SRC_PRESENTATION_IMAGE_IMAGE_H_
#define AVALON_SRC_PRESENTATION_IMAGE_IMAGE_H_

#include <Avalon/src/device/Device.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vulkan/vulkan.hpp>

class Image {
 private:
  VkImage m_image;
  VkDeviceMemory m_imageMemory;
  VkImageView m_imageView;

  std::shared_ptr<Device> m_device;
  //  std::shared_ptr<VmaAllocator> m_allocator;

 public:
  Image();
  virtual ~Image();

  void cleanUp();

 public:
  inline VkImage& getImage() { return m_image; }
  inline VkDeviceMemory& getImageMemory() { return m_imageMemory; }
  inline VkImageView& getImageView() { return m_imageView; }
};

#endif  // AVALON_SRC_PRESENTATION_IMAGE_IMAGE_H_

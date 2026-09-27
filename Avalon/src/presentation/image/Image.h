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

#include <Avalon/src/allocator/VmaAllocator.h>
#include <Avalon/src/device/Device.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vulkan/vulkan.hpp>

namespace Camelot {
struct ImageCreateInfo {
  int width{0};
  int height{0};
  int mipLevels{1};
  VkSampleCountFlagBits numSample{VK_SAMPLE_COUNT_1_BIT};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
  VkImageUsageFlags usage{0};
  VkMemoryPropertyFlags properties{0};
  VkImageAspectFlags aspectFlags{0};
};
}  // namespace Camelot

class Image {
 private:
  VkImage m_image{VK_NULL_HANDLE};
  VkImageView m_imageView{VK_NULL_HANDLE};

  VmaAllocation m_allocation{VK_NULL_HANDLE};

  std::shared_ptr<Device> m_device;
  std::shared_ptr<VmaAllocatorWrapper> m_allocator;

 public:
  Image(std::shared_ptr<Device> device,
        std::shared_ptr<VmaAllocatorWrapper> allocator);
  Image(const Image&) = delete;
  Image(Image&&) noexcept;
  Image& operator=(const Image&) = delete;
  Image& operator=(Image&&) noexcept;

  virtual ~Image();

  void cleanUp();
  void createImage(const Camelot::ImageCreateInfo&);

 public:
  inline VkImage& getImage() { return m_image; }
  inline VkImageView& getImageView() { return m_imageView; }
};

#endif  // AVALON_SRC_PRESENTATION_IMAGE_IMAGE_H_

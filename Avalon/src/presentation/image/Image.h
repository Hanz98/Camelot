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
#include <Avalon/src/utils/ResourceDescriptor.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vulkan/vulkan.hpp>

namespace Camelot {
struct ImageCreateInfo {
  int width;
  int height;
  int mipLevels;
  VkSampleCountFlagBits numSample;
  VkFormat format;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  VkMemoryPropertyFlags properties;
  VkImageAspectFlags aspectFlags;
};
}  // namespace Camelot

class Image {
 private:
  VkImage m_image;
  VkImageView m_imageView;

  VmaAllocation m_allocation;
  std::shared_ptr<Resource::Descriptor> m_resourceDescriptor;

 public:
  Image();
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

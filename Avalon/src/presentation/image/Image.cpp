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
#include "Image.h"

#include <Avalon/src/utils/ResourceDescriptor.h>
#include <Avalon/src/validation/CheckResult.h>
#include <vma/vk_mem_alloc.h>

#include <utility>

Image::Image()
    : m_device(Resource::Descriptor->device),
      m_image(VK_NULL_HANDLE),
      m_imageView(VK_NULL_HANDLE),
      m_allocation(VK_NULL_HANDLE),
      m_allocator(Resource::Descriptor->allocator) {}

Image::Image(Image&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_image(std::exchange(other.m_image, VK_NULL_HANDLE)),
      m_imageView(std::exchange(other.m_imageView, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_allocator(std::move(other.m_allocator)) {}

Image& Image::operator=(Image&& other) noexcept {
  if (this != &other) {
    m_device = std::move(other.m_device);
    m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
    m_imageView = std::exchange(other.m_imageView, VK_NULL_HANDLE);
    m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
    m_allocator = std::move(other.m_allocator);
  }
  return *this;
}

Image::~Image() { cleanUp(); }

void Image::cleanUp() {
  if (m_imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(m_device->getDevice(), m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;
  }

  if (m_image != VK_NULL_HANDLE) {
    vmaDestroyImage(m_allocator->allocator, m_image, m_allocation);
    m_image = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
  }
}

void Image::createImage(const Camelot::ImageCreateInfo& createInfo) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = createInfo.width;
  imageInfo.extent.height = createInfo.height;
  imageInfo.mipLevels = createInfo.mipLevels;
  imageInfo.extent.depth = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = createInfo.format;
  imageInfo.tiling = createInfo.tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = createInfo.usage;
  imageInfo.samples = createInfo.numSample;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaCreateInfo = {};
  vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  VK_CHECK_RESULT(vmaCreateImage(m_allocator->allocator, &imageInfo,
                                 &vmaCreateInfo, &m_image, &m_allocation,
                                 nullptr));

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = m_image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = createInfo.format;
  viewInfo.subresourceRange.aspectMask = createInfo.aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = createInfo.mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VK_CHECK_RESULT(vkCreateImageView(m_device->getDevice(), &viewInfo, nullptr,
                                    &m_imageView));
}

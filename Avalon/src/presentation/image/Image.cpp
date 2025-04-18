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

Image::Image()
    : m_device(Resource::Descriptor->device),
      m_image(VK_NULL_HANDLE),
      m_imageMemory(VK_NULL_HANDLE),
      m_imageView(VK_NULL_HANDLE) {}

Image::~Image() { cleanUp(); }

void Image::cleanUp() {
  if (m_imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(m_device->getDevice(), m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;
  }

  if (m_image != VK_NULL_HANDLE) {
    vkDestroyImage(m_device->getDevice(), m_image, nullptr);
    m_image = VK_NULL_HANDLE;
  }

  if (m_imageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(m_device->getDevice(), m_imageMemory, nullptr);
    m_imageMemory = VK_NULL_HANDLE;
  }
}

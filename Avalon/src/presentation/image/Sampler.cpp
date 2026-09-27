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

#include "Avalon/src/presentation/image/Sampler.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <utility>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

Sampler::Sampler(std::shared_ptr<Device> device, VkFilter filter,
                 VkSamplerAddressMode addressMode)
    : m_device(std::move(device)) {
  if (m_device == nullptr) {
    spdlog::error("Sampler: device is not initialized.");
    throw std::runtime_error("Sampler: device is not initialized.");
  }
  VkSamplerCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.magFilter = filter;
  info.minFilter = filter;
  info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  info.addressModeU = addressMode;
  info.addressModeV = addressMode;
  info.addressModeW = addressMode;
  info.anisotropyEnable = VK_FALSE;
  info.maxAnisotropy = 1.0F;
  info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  info.unnormalizedCoordinates = VK_FALSE;
  info.compareEnable = VK_FALSE;
  info.minLod = 0.0F;
  info.maxLod = 0.0F;
  VK_CHECK_RESULT(
      vkCreateSampler(m_device->getDevice(), &info, nullptr, &m_sampler));
}

Sampler::Sampler(Sampler&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_sampler(std::exchange(other.m_sampler, VK_NULL_HANDLE)) {}

Sampler& Sampler::operator=(Sampler&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = std::move(other.m_device);
    m_sampler = std::exchange(other.m_sampler, VK_NULL_HANDLE);
  }
  return *this;
}

Sampler::~Sampler() { cleanUp(); }

void Sampler::cleanUp() {
  if (m_sampler != VK_NULL_HANDLE && m_device != nullptr) {
    vkDestroySampler(m_device->getDevice(), m_sampler, nullptr);
  }
  m_sampler = VK_NULL_HANDLE;
}

}  // namespace avalon

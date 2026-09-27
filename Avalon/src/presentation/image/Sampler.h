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

#ifndef AVALON_SRC_PRESENTATION_IMAGE_SAMPLER_H_
#define AVALON_SRC_PRESENTATION_IMAGE_SAMPLER_H_

#include <vulkan/vulkan.h>

#include <memory>

#include "Avalon/src/device/Device.h"

namespace avalon {

// RAII VkSampler with the settings textures shown in the UI need.
class Sampler {
 private:
  std::shared_ptr<Device> m_device;
  VkSampler m_sampler{VK_NULL_HANDLE};

 public:
  explicit Sampler(
      std::shared_ptr<Device> device, VkFilter filter = VK_FILTER_LINEAR,
      VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
  Sampler(const Sampler&) = delete;
  Sampler& operator=(const Sampler&) = delete;
  Sampler(Sampler&& other) noexcept;
  Sampler& operator=(Sampler&& other) noexcept;
  ~Sampler();

  void cleanUp();

  [[nodiscard]] VkSampler get() const { return m_sampler; }
};

}  // namespace avalon

#endif  // AVALON_SRC_PRESENTATION_IMAGE_SAMPLER_H_

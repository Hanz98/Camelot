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

#include "Avalon/src/pipeline/PipelineLayout.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

PipelineLayout::PipelineLayout(
    std::shared_ptr<Device> device, std::string name,
    const std::vector<VkDescriptorSetLayout>& setLayouts,
    const std::vector<VkPushConstantRange>& pushConstants)
    : m_device(std::move(device)), m_name(std::move(name)) {
  if (m_device == nullptr) {
    spdlog::error("PipelineLayout '{}': device is not initialized.", m_name);
    throw std::runtime_error("PipelineLayout: device is not initialized.");
  }
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
  info.pSetLayouts = setLayouts.data();
  info.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
  info.pPushConstantRanges = pushConstants.data();
  VK_CHECK_RESULT(
      vkCreatePipelineLayout(m_device->getDevice(), &info, nullptr, &m_layout));
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_layout(std::exchange(other.m_layout, VK_NULL_HANDLE)),
      m_name(std::move(other.m_name)) {}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = std::move(other.m_device);
    m_layout = std::exchange(other.m_layout, VK_NULL_HANDLE);
    m_name = std::move(other.m_name);
  }
  return *this;
}

PipelineLayout::~PipelineLayout() { cleanUp(); }

void PipelineLayout::cleanUp() {
  if (m_layout != VK_NULL_HANDLE && m_device != nullptr) {
    vkDestroyPipelineLayout(m_device->getDevice(), m_layout, nullptr);
  }
  m_layout = VK_NULL_HANDLE;
}

}  // namespace avalon

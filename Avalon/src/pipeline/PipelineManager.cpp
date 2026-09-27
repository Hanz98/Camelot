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

#include "Avalon/src/pipeline/PipelineManager.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace avalon {

PipelineManager::PipelineManager(std::shared_ptr<Device> device)
    : m_device(std::move(device)) {
  if (m_device == nullptr) {
    spdlog::error("PipelineManager: device is not initialized.");
    throw std::runtime_error("PipelineManager: device is not initialized.");
  }
}

PipelineManager::~PipelineManager() { clear(); }

void PipelineManager::clear() {
  // Pipelines reference layouts, so they go first.
  m_pipelines.clear();
  m_layouts.clear();
}

PipelineLayout& PipelineManager::getOrCreateLayout(
    const std::string& name,
    const std::vector<VkDescriptorSetLayout>& setLayouts,
    const std::vector<VkPushConstantRange>& pushConstants) {
  auto it = m_layouts.find(name);
  if (it == m_layouts.end()) {
    it = m_layouts
             .emplace(name, std::make_unique<PipelineLayout>(
                                m_device, name, setLayouts, pushConstants))
             .first;
  }
  return *it->second;
}

GraphicsPipeline& PipelineManager::getOrCreate(const std::string& name,
                                               const Factory& factory) {
  auto it = m_pipelines.find(name);
  if (it == m_pipelines.end()) {
    it = m_pipelines
             .emplace(name, std::make_unique<GraphicsPipeline>(factory(*this)))
             .first;
  }
  return *it->second;
}

GraphicsPipeline& PipelineManager::get(const std::string& name) const {
  auto it = m_pipelines.find(name);
  if (it == m_pipelines.end()) {
    throw std::out_of_range("PipelineManager: no pipeline named '" + name +
                            "'.");
  }
  return *it->second;
}

PipelineLayout& PipelineManager::getLayout(const std::string& name) const {
  auto it = m_layouts.find(name);
  if (it == m_layouts.end()) {
    throw std::out_of_range("PipelineManager: no pipeline layout named '" +
                            name + "'.");
  }
  return *it->second;
}

bool PipelineManager::has(const std::string& name) const {
  return m_pipelines.contains(name);
}

void PipelineManager::remove(const std::string& name) {
  m_pipelines.erase(name);
}

}  // namespace avalon

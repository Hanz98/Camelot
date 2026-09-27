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

#include "Avalon/src/shader/ShaderModule.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Avalon/src/validation/CheckResult.h"

namespace avalon {

namespace {

constexpr uint32_t kSpirvMagic = 0x07230203U;

}  // namespace

ShaderModule::ShaderModule(std::shared_ptr<Device> device,
                           std::span<const uint32_t> code,
                           VkShaderStageFlagBits stage, std::string name)
    : m_device(std::move(device)), m_stage(stage), m_name(std::move(name)) {
  if (m_device == nullptr) {
    throw std::runtime_error("ShaderModule '" + m_name +
                             "': Device is not initialized.");
  }
  if (code.empty()) {
    throw std::runtime_error("ShaderModule '" + m_name + "': empty SPIR-V.");
  }
  if (code[0] != kSpirvMagic) {
    throw std::runtime_error("ShaderModule '" + m_name +
                             "': bad SPIR-V magic number.");
  }

  VkShaderModuleCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = code.size_bytes();
  info.pCode = code.data();
  VK_CHECK_RESULT(
      vkCreateShaderModule(m_device->getDevice(), &info, nullptr, &m_module));
  spdlog::debug("ShaderModule '{}' created ({} words).", m_name, code.size());
}

ShaderModule::ShaderModule(std::shared_ptr<Device> device,
                           const avalon::shaders::ShaderBlob& blob)
    : ShaderModule(std::move(device), blob.code, blob.stage,
                   std::string(blob.name)) {}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    : m_device(std::move(other.m_device)),
      m_module(std::exchange(other.m_module, VK_NULL_HANDLE)),
      m_stage(other.m_stage),
      m_name(std::move(other.m_name)) {}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept {
  if (this != &other) {
    cleanUp();
    m_device = std::move(other.m_device);
    m_module = std::exchange(other.m_module, VK_NULL_HANDLE);
    m_stage = other.m_stage;
    m_name = std::move(other.m_name);
  }
  return *this;
}

ShaderModule::~ShaderModule() { cleanUp(); }

void ShaderModule::cleanUp() {
  if (m_module == VK_NULL_HANDLE || m_device == nullptr) {
    return;
  }
  VkDevice device = m_device->getDevice();
  if (device != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, m_module, nullptr);
  }
  m_module = VK_NULL_HANDLE;
}

ShaderModule ShaderModule::fromFile(std::shared_ptr<Device> device,
                                    const std::filesystem::path& path,
                                    VkShaderStageFlagBits stage) {
  if (stage == VK_SHADER_STAGE_ALL) {
    stage = stageFromFileName(path);
  }
  std::vector<uint32_t> code = readSpirv(path);
  return {std::move(device), code, stage, path.filename().string()};
}

std::vector<uint32_t> ShaderModule::readSpirv(
    const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error("ShaderModule: cannot open SPIR-V file '" +
                             path.string() + "'.");
  }
  const std::streamoff size = file.tellg();
  if (size <= 0 || size % sizeof(uint32_t) != 0) {
    throw std::runtime_error("ShaderModule: '" + path.string() +
                             "' is not a SPIR-V module (size " +
                             std::to_string(size) + ").");
  }
  std::vector<uint32_t> code(static_cast<size_t>(size) / sizeof(uint32_t));
  file.seekg(0);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  file.read(reinterpret_cast<char*>(code.data()), size);
  if (!file) {
    throw std::runtime_error("ShaderModule: failed to read '" + path.string() +
                             "'.");
  }
  if (code[0] != kSpirvMagic) {
    throw std::runtime_error("ShaderModule: '" + path.string() +
                             "' has a bad SPIR-V magic number.");
  }
  return code;
}

VkShaderStageFlagBits ShaderModule::stageFromFileName(
    const std::filesystem::path& path) {
  std::filesystem::path stem = path;
  if (stem.extension() == ".spv") {
    stem.replace_extension();
  }
  const std::string ext = stem.extension().string();
  if (ext == ".vert") return VK_SHADER_STAGE_VERTEX_BIT;
  if (ext == ".frag") return VK_SHADER_STAGE_FRAGMENT_BIT;
  if (ext == ".comp") return VK_SHADER_STAGE_COMPUTE_BIT;
  if (ext == ".geom") return VK_SHADER_STAGE_GEOMETRY_BIT;
  if (ext == ".tesc") return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  if (ext == ".tese") return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  throw std::runtime_error("ShaderModule: cannot infer shader stage from '" +
                           path.string() + "'.");
}

VkPipelineShaderStageCreateInfo ShaderModule::stageInfo(
    const char* entryPoint) const {
  VkPipelineShaderStageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = m_stage;
  info.module = m_module;
  info.pName = entryPoint;
  return info;
}

}  // namespace avalon

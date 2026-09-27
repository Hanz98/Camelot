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

#ifndef AVALON_SRC_SHADER_SHADERMODULE_H_
#define AVALON_SRC_SHADER_SHADERMODULE_H_

#include <vulkan/vulkan.h>

#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/device/Device.h"

namespace avalon {

// RAII wrapper around VkShaderModule. Modules are usually created from the
// shaders embedded at build time (avalon::shaders::get("name.vert")); loading
// a .spv file from disk is supported for tools and tests.
class ShaderModule {
 private:
  std::shared_ptr<Device> m_device;
  VkShaderModule m_module = VK_NULL_HANDLE;
  VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_ALL;
  std::string m_name;

 public:
  // `code` is copied into the driver by vkCreateShaderModule and need not
  // outlive the call. `name` is only used in log and error messages.
  ShaderModule(std::shared_ptr<Device> device, std::span<const uint32_t> code,
               VkShaderStageFlagBits stage, std::string name);
  ShaderModule(std::shared_ptr<Device> device,
               const avalon::shaders::ShaderBlob& blob);
  ShaderModule(const ShaderModule&) = delete;
  ShaderModule& operator=(const ShaderModule&) = delete;
  ShaderModule(ShaderModule&& other) noexcept;
  ShaderModule& operator=(ShaderModule&& other) noexcept;
  ~ShaderModule();

  void cleanUp();

  // Loads a SPIR-V file. The stage is inferred from the file name when not
  // given ("triangle.vert.spv" -> vertex). Throws std::runtime_error when the
  // file is missing, empty, or not a SPIR-V module.
  static ShaderModule fromFile(
      std::shared_ptr<Device> device, const std::filesystem::path& path,
      VkShaderStageFlagBits stage = VK_SHADER_STAGE_ALL);

  // Reads and validates a SPIR-V file (size, magic number).
  static std::vector<uint32_t> readSpirv(const std::filesystem::path& path);

  // Maps a GLSL stage extension (.vert, .frag, .comp, .geom, .tesc, .tese) to
  // the Vulkan stage; a trailing ".spv" is ignored. Throws std::runtime_error
  // for anything else.
  static VkShaderStageFlagBits stageFromFileName(
      const std::filesystem::path& path);

  [[nodiscard]] VkShaderModule get() const { return m_module; }
  [[nodiscard]] VkShaderStageFlagBits stage() const { return m_stage; }
  [[nodiscard]] const std::string& name() const { return m_name; }

  // Fills a pipeline stage description for this module. `entryPoint` must
  // outlive the returned struct.
  [[nodiscard]] VkPipelineShaderStageCreateInfo stageInfo(
      const char* entryPoint = "main") const;
};

}  // namespace avalon

#endif  // AVALON_SRC_SHADER_SHADERMODULE_H_

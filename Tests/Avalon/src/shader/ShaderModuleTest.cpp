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

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/shader/ShaderModule.h"

namespace avalon {

namespace {

constexpr uint32_t kSpirvMagic = 0x07230203U;

std::filesystem::path compiledShader(const char* name) {
  return std::filesystem::path(CAMELOT_TEST_SHADER_DIR) / name;
}

}  // namespace

TEST(ShaderRegistryTest, TriangleShadersAreEmbedded) {
  EXPECT_EQ(avalon::shaders::all().size(), 2U);

  const avalon::shaders::ShaderBlob* vert =
      avalon::shaders::find("triangle.vert");
  ASSERT_NE(vert, nullptr);
  EXPECT_EQ(vert->stage, VK_SHADER_STAGE_VERTEX_BIT);
  ASSERT_FALSE(vert->code.empty());
  EXPECT_EQ(vert->code[0], kSpirvMagic);

  const avalon::shaders::ShaderBlob& frag =
      avalon::shaders::get("triangle.frag");
  EXPECT_EQ(frag.stage, VK_SHADER_STAGE_FRAGMENT_BIT);
  EXPECT_EQ(frag.code[0], kSpirvMagic);
}

TEST(ShaderRegistryTest, UnknownShaderIsReported) {
  EXPECT_EQ(avalon::shaders::find("missing.vert"), nullptr);
  EXPECT_THROW(avalon::shaders::get("missing.vert"), std::out_of_range);
}

TEST(ShaderRegistryTest, EmbeddedCodeMatchesCompiledFile) {
  const std::vector<uint32_t> fromDisk =
      ShaderModule::readSpirv(compiledShader("triangle.vert.spv"));
  const auto embedded = avalon::shaders::get("triangle.vert").code;
  ASSERT_EQ(fromDisk.size(), embedded.size());
  EXPECT_TRUE(std::equal(fromDisk.begin(), fromDisk.end(), embedded.begin()));
}

TEST(ShaderModuleTest, StageIsInferredFromFileName) {
  EXPECT_EQ(ShaderModule::stageFromFileName("a.vert"),
            VK_SHADER_STAGE_VERTEX_BIT);
  EXPECT_EQ(ShaderModule::stageFromFileName("dir/a.frag.spv"),
            VK_SHADER_STAGE_FRAGMENT_BIT);
  EXPECT_EQ(ShaderModule::stageFromFileName("a.comp"),
            VK_SHADER_STAGE_COMPUTE_BIT);
  EXPECT_THROW(ShaderModule::stageFromFileName("a.spv"), std::runtime_error);
  EXPECT_THROW(ShaderModule::stageFromFileName("a.glsl"), std::runtime_error);
}

TEST(ShaderModuleTest, ReadSpirvRejectsMissingFile) {
  EXPECT_THROW(ShaderModule::readSpirv(compiledShader("does-not-exist.spv")),
               std::runtime_error);
}

TEST(ShaderModuleTest, ReadSpirvRejectsNonSpirvFile) {
  const std::filesystem::path bogus =
      std::filesystem::temp_directory_path() / "camelot-bogus.frag.spv";
  {
    std::ofstream out(bogus, std::ios::binary);
    out << "not spir-v";  // 10 bytes: not a multiple of 4
  }
  EXPECT_THROW(ShaderModule::readSpirv(bogus), std::runtime_error);
  {
    std::ofstream out(bogus, std::ios::binary);
    out << "12345678";  // right size, wrong magic
  }
  EXPECT_THROW(ShaderModule::readSpirv(bogus), std::runtime_error);
  std::filesystem::remove(bogus);
}

// Module creation needs a device. Runs on the mock ICD in CI.
class ShaderModuleDeviceTest : public testing::Test {
 protected:
  Avalon avalon;

  void SetUp() override { avalon.init(); }
  void TearDown() override { avalon.cleanUp(); }
};

TEST_F(ShaderModuleDeviceTest, CreatesModuleFromEmbeddedShader) {
  ShaderModule module(avalon.getDevice(),
                      avalon::shaders::get("triangle.vert"));
  EXPECT_NE(module.get(), VK_NULL_HANDLE);
  EXPECT_EQ(module.stage(), VK_SHADER_STAGE_VERTEX_BIT);
  EXPECT_EQ(module.name(), "triangle.vert");

  const VkPipelineShaderStageCreateInfo info = module.stageInfo();
  EXPECT_EQ(info.sType, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
  EXPECT_EQ(info.stage, VK_SHADER_STAGE_VERTEX_BIT);
  EXPECT_EQ(info.module, module.get());
  EXPECT_STREQ(info.pName, "main");
}

TEST_F(ShaderModuleDeviceTest, LoadsModuleFromCompiledFile) {
  ShaderModule module = ShaderModule::fromFile(
      avalon.getDevice(), compiledShader("triangle.frag.spv"));
  EXPECT_NE(module.get(), VK_NULL_HANDLE);
  EXPECT_EQ(module.stage(), VK_SHADER_STAGE_FRAGMENT_BIT);
  EXPECT_EQ(module.name(), "triangle.frag.spv");
}

TEST_F(ShaderModuleDeviceTest, RejectsInvalidCode) {
  const std::vector<uint32_t> garbage = {1, 2, 3};
  EXPECT_THROW(ShaderModule(avalon.getDevice(), garbage,
                            VK_SHADER_STAGE_VERTEX_BIT, "garbage"),
               std::runtime_error);
  EXPECT_THROW(ShaderModule(avalon.getDevice(), std::vector<uint32_t>{},
                            VK_SHADER_STAGE_VERTEX_BIT, "empty"),
               std::runtime_error);
}

TEST_F(ShaderModuleDeviceTest, MoveTransfersOwnership) {
  ShaderModule first(avalon.getDevice(), avalon::shaders::get("triangle.frag"));
  const VkShaderModule handle = first.get();
  ASSERT_NE(handle, VK_NULL_HANDLE);

  ShaderModule second(std::move(first));
  EXPECT_EQ(second.get(), handle);
  EXPECT_EQ(first.get(), VK_NULL_HANDLE);  // NOLINT(bugprone-use-after-move)

  ShaderModule third(avalon.getDevice(), avalon::shaders::get("triangle.vert"));
  third = std::move(second);
  EXPECT_EQ(third.get(), handle);
  EXPECT_EQ(third.stage(), VK_SHADER_STAGE_FRAGMENT_BIT);
}

}  // namespace avalon

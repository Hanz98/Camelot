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

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Avalon/shaders/Registry.h"
#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/pipeline/GraphicsPipeline.h"
#include "Avalon/src/pipeline/PipelineLayout.h"
#include "Avalon/src/pipeline/PipelineManager.h"
#include "Avalon/src/renderer/Renderer.h"
#include "Avalon/src/shader/ShaderModule.h"

namespace avalon {

// Builds pipelines on the mock ICD through a fully initialised engine.
class PipelineTest : public testing::Test {
 protected:
  Avalon avalon;
  std::unique_ptr<ShaderModule> vert;
  std::unique_ptr<ShaderModule> frag;

  void SetUp() override {
    avalon.init();
    vert =
        std::make_unique<ShaderModule>(device(), shaders::get("triangle.vert"));
    frag =
        std::make_unique<ShaderModule>(device(), shaders::get("triangle.frag"));
  }
  void TearDown() override {
    frag.reset();
    vert.reset();
    avalon.cleanUp();
  }

  [[nodiscard]] std::shared_ptr<Device> device() const {
    return avalon.getDevice();
  }
  [[nodiscard]] VkRenderPass renderPass() const {
    return avalon.getRenderer()->getRenderPass().getRenderPass();
  }
  [[nodiscard]] GraphicsPipelineBuilder triangleBuilder(
      const std::string& name, VkPipelineLayout layout) const {
    return GraphicsPipelineBuilder(name)
        .addStage(*vert)
        .addStage(*frag)
        .setRenderPass(renderPass())
        .setLayout(layout);
  }
};

TEST_F(PipelineTest, LayoutIsCreatedAndReleased) {
  PipelineLayout layout(device(), "empty");
  EXPECT_NE(layout.get(), VK_NULL_HANDLE);
  EXPECT_EQ(layout.name(), "empty");
  layout.cleanUp();
  EXPECT_EQ(layout.get(), VK_NULL_HANDLE);
}

TEST_F(PipelineTest, LayoutMoveTransfersOwnership) {
  PipelineLayout first(device(), "moved");
  const VkPipelineLayout handle = first.get();
  PipelineLayout second(std::move(first));
  EXPECT_EQ(second.get(), handle);
  EXPECT_EQ(first.get(), VK_NULL_HANDLE);  // NOLINT(bugprone-use-after-move)
}

TEST_F(PipelineTest, RendererBuildsTheTrianglePipeline) {
  const PipelineManager& pm = avalon.getRenderer()->getPipelineManager();
  ASSERT_TRUE(pm.has(Renderer::kTrianglePipeline));
  const GraphicsPipeline& triangle = pm.get(Renderer::kTrianglePipeline);
  EXPECT_NE(triangle.get(), VK_NULL_HANDLE);
  EXPECT_NE(triangle.getLayout(), VK_NULL_HANDLE);
  EXPECT_EQ(triangle.name(), Renderer::kTrianglePipeline);
  // Draw a frame that binds it.
  EXPECT_TRUE(avalon.frame());
}

TEST_F(PipelineTest, BuilderBuildsAPipeline) {
  PipelineLayout layout(device(), "empty");
  GraphicsPipeline pipeline =
      triangleBuilder("test", layout.get())
          .setTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
          .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
          .setDepth(false, false)
          .setAlphaBlend(true)
          .build(device());
  EXPECT_NE(pipeline.get(), VK_NULL_HANDLE);
  EXPECT_EQ(pipeline.getLayout(), layout.get());
}

TEST_F(PipelineTest, BuilderRejectsIncompleteDescriptionsNamingThePipeline) {
  PipelineLayout layout(device(), "empty");
  auto expectThrowNaming = [this](const GraphicsPipelineBuilder& builder) {
    try {
      (void)builder.build(device());
      FAIL() << "expected build() to throw";
    } catch (const std::runtime_error& e) {
      EXPECT_NE(std::string(e.what()).find("incomplete"), std::string::npos)
          << e.what();
    }
  };
  expectThrowNaming(GraphicsPipelineBuilder("incomplete")
                        .setRenderPass(renderPass())
                        .setLayout(layout.get()));  // no stages
  expectThrowNaming(GraphicsPipelineBuilder("incomplete")
                        .addStage(*vert)
                        .setLayout(layout.get()));  // no render pass
  expectThrowNaming(GraphicsPipelineBuilder("incomplete")
                        .addStage(*vert)
                        .setRenderPass(renderPass()));  // no layout
}

TEST_F(PipelineTest, ManagerCachesByName) {
  PipelineManager pm(device());
  int builds = 0;
  auto factory = [&](PipelineManager& m) {
    ++builds;
    return triangleBuilder("cached", m.getOrCreateLayout("empty").get())
        .build(device());
  };
  GraphicsPipeline& first = pm.getOrCreate("cached", factory);
  GraphicsPipeline& second = pm.getOrCreate("cached", factory);
  EXPECT_EQ(&first, &second);
  EXPECT_EQ(builds, 1);
  EXPECT_EQ(pm.size(), 1U);
  EXPECT_TRUE(pm.has("cached"));
  EXPECT_EQ(&pm.get("cached"), &first);
  EXPECT_EQ(&pm.getLayout("empty"), &pm.getOrCreateLayout("empty"));

  pm.remove("cached");
  EXPECT_FALSE(pm.has("cached"));
  pm.getOrCreate("cached", factory);
  EXPECT_EQ(builds, 2);

  pm.clear();
  EXPECT_EQ(pm.size(), 0U);
}

TEST_F(PipelineTest, ManagerNamesMissingPipelines) {
  PipelineManager pm(device());
  EXPECT_THROW(
      {
        try {
          (void)pm.get("nope");
        } catch (const std::out_of_range& e) {
          EXPECT_NE(std::string(e.what()).find("nope"), std::string::npos);
          throw;
        }
      },
      std::out_of_range);
  EXPECT_THROW((void)pm.getLayout("nope"), std::out_of_range);
}

}  // namespace avalon

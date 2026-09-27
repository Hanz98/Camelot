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
#include <imgui.h>

#include <vector>

#include "Avalon/src/main/Avalon.h"
#include "Avalon/src/ui/GraphWidget.h"

namespace avalon {

TEST(GraphWidgetTest, RingBufferKeepsTheNewestSamples) {
  GraphWidget graph("g", 4);
  EXPECT_TRUE(graph.empty());
  EXPECT_EQ(graph.capacity(), 4U);
  EXPECT_EQ(graph.latest(), 0.0F);
  for (int v = 1; v <= 6; ++v) {
    graph.push(static_cast<float>(v));
  }
  EXPECT_EQ(graph.size(), 4U);
  EXPECT_EQ(graph.latest(), 6.0F);
  EXPECT_EQ(graph.ordered(), (std::vector<float>{3.0F, 4.0F, 5.0F, 6.0F}));
  EXPECT_FLOAT_EQ(graph.average(), 4.5F);
  graph.clear();
  EXPECT_TRUE(graph.empty());
  EXPECT_EQ(graph.ordered().size(), 0U);
}

TEST(GraphWidgetTest, CapacityIsAtLeastTwo) {
  const GraphWidget graph("tiny", 0);
  EXPECT_EQ(graph.capacity(), 2U);
  EXPECT_EQ(graph.title(), "tiny");
}

TEST(GraphWidgetTest, DrawsInsideAFrame) {
  Avalon avalon;
  avalon.init();
  GraphWidget graph("frame time", 64, "ms");
  avalon.setUiCallback([&]() {
    graph.push(16.7F);
    ImGui::Begin("graph");
    graph.draw(120.0F);
    graph.draw();  // default height
    ImGui::End();
  });
  EXPECT_TRUE(avalon.frame());
  EXPECT_TRUE(avalon.frame());
  EXPECT_EQ(graph.size(), 2U);
  avalon.cleanUp();
}

}  // namespace avalon

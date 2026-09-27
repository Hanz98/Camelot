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

#ifndef AVALON_SRC_UI_GRAPHWIDGET_H_
#define AVALON_SRC_UI_GRAPHWIDGET_H_

#include <cstddef>
#include <string>
#include <vector>

namespace avalon {

// A rolling time series drawn as an ImPlot line: push() appends a sample,
// the oldest one falls out once `capacity` is reached. The buffer logic
// works without an ImGui context; only draw() needs one.
class GraphWidget {
 private:
  std::string m_title;
  std::string m_yLabel;
  std::vector<float> m_values;  // ring buffer
  size_t m_head{0};             // next write position
  size_t m_size{0};
  double m_nextX{0.0};
  std::vector<float> m_xs;  // scratch for drawing
  std::vector<float> m_ys;

 public:
  explicit GraphWidget(std::string title, size_t capacity = 512,
                       std::string yLabel = "");

  void push(float value);
  void clear();

  // Draws the plot filling the available content region of the current
  // ImGui window. Height <= 0 uses the ImPlot default.
  void draw(float height = 0.0F);

  [[nodiscard]] size_t size() const { return m_size; }
  [[nodiscard]] size_t capacity() const { return m_values.size(); }
  [[nodiscard]] bool empty() const { return m_size == 0; }
  [[nodiscard]] float latest() const;
  [[nodiscard]] float average() const;
  // Samples oldest to newest.
  [[nodiscard]] std::vector<float> ordered() const;
  [[nodiscard]] const std::string& title() const { return m_title; }
};

}  // namespace avalon

#endif  // AVALON_SRC_UI_GRAPHWIDGET_H_

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

#include "Avalon/src/ui/GraphWidget.h"

#include <imgui.h>
#include <implot.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace avalon {

GraphWidget::GraphWidget(std::string title, size_t capacity, std::string yLabel)
    : m_title(std::move(title)),
      m_yLabel(std::move(yLabel)),
      m_values(std::max<size_t>(capacity, 2), 0.0F) {}

void GraphWidget::push(float value) {
  m_values[m_head] = value;
  m_head = (m_head + 1) % m_values.size();
  m_size = std::min(m_size + 1, m_values.size());
  m_nextX += 1.0;
}

void GraphWidget::clear() {
  m_head = 0;
  m_size = 0;
  m_nextX = 0.0;
}

float GraphWidget::latest() const {
  if (m_size == 0) {
    return 0.0F;
  }
  const size_t last = (m_head + m_values.size() - 1) % m_values.size();
  return m_values[last];
}

float GraphWidget::average() const {
  if (m_size == 0) {
    return 0.0F;
  }
  float sum = 0.0F;
  for (const float value : ordered()) {
    sum += value;
  }
  return sum / static_cast<float>(m_size);
}

std::vector<float> GraphWidget::ordered() const {
  std::vector<float> out;
  out.reserve(m_size);
  const size_t start = (m_head + m_values.size() - m_size) % m_values.size();
  for (size_t i = 0; i < m_size; ++i) {
    out.push_back(m_values[(start + i) % m_values.size()]);
  }
  return out;
}

void GraphWidget::draw(float height) {
  m_ys = ordered();
  m_xs.resize(m_ys.size());
  const double firstX = m_nextX - static_cast<double>(m_ys.size());
  for (size_t i = 0; i < m_xs.size(); ++i) {
    m_xs[i] = static_cast<float>(firstX + static_cast<double>(i));
  }
  const ImVec2 size(-1.0F, height > 0.0F ? height : 0.0F);
  if (ImPlot::BeginPlot(m_title.c_str(), size, ImPlotFlags_NoLegend)) {
    ImPlot::SetupAxes("sample", m_yLabel.empty() ? nullptr : m_yLabel.c_str(),
                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    if (!m_ys.empty()) {
      ImPlot::PlotLine(m_title.c_str(), m_xs.data(), m_ys.data(),
                       static_cast<int>(m_ys.size()));
    }
    ImPlot::EndPlot();
  }
  ImGui::TextUnformatted(fmt::format("latest {:.3f}  avg {:.3f}  ({} samples)",
                                     latest(), average(), m_size)
                             .c_str());
}

}  // namespace avalon

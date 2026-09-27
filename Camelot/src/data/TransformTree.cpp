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

#include "Camelot/src/data/TransformTree.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace camelot {

namespace {

constexpr double kZeroNormEpsilon = 1e-12;

}  // namespace

glm::mat4 TransformTree::toMatrix(const Sample& sample) {
  const Quat& q = sample.rotation;
  const double norm = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
  glm::mat4 rotation(1.0F);
  if (norm > kZeroNormEpsilon) {
    // glm::quat takes (w, x, y, z).
    const glm::quat quat = glm::normalize(
        glm::quat(static_cast<float>(q.w), static_cast<float>(q.x),
                  static_cast<float>(q.y), static_cast<float>(q.z)));
    rotation = glm::mat4_cast(quat);
  }
  const Vec3& p = sample.translation;
  const glm::mat4 translation =
      glm::translate(glm::mat4(1.0F),
                     glm::vec3(static_cast<float>(p.x), static_cast<float>(p.y),
                               static_cast<float>(p.z)));
  return translation * rotation;
}

void TransformTree::add(const FrameTransform& transform) {
  Edge& edge = m_edges[transform.childFrameId];
  if (edge.parent != transform.parentFrameId) {
    edge.parent = transform.parentFrameId;
    edge.samples.clear();
  }
  const Sample sample{.time = transform.timestamp,
                      .translation = transform.translation,
                      .rotation = transform.rotation};
  auto at =
      std::ranges::lower_bound(edge.samples, sample.time, {}, &Sample::time);
  if (at != edge.samples.end() && at->time == sample.time) {
    *at = sample;
  } else {
    edge.samples.insert(at, sample);
  }
}

const TransformTree::Edge* TransformTree::edge(std::string_view child) const {
  const auto found = m_edges.find(child);
  return found == m_edges.end() ? nullptr : &found->second;
}

std::vector<std::pair<std::string_view, const TransformTree::Edge*>>
TransformTree::chain(std::string_view frame) const {
  std::vector<std::pair<std::string_view, const Edge*>> out;
  std::string_view current = frame;
  // A malformed tree could contain a cycle; the chain can never be longer
  // than the number of edges.
  while (out.size() <= m_edges.size()) {
    const Edge* next = edge(current);
    if (next == nullptr) {
      break;
    }
    out.emplace_back(current, next);
    current = next->parent;
  }
  return out;
}

const TransformTree::Sample& TransformTree::nearest(const Edge& edge, Time t) {
  const auto after =
      std::ranges::lower_bound(edge.samples, t, {}, &Sample::time);
  if (after == edge.samples.begin()) {
    return *after;
  }
  if (after == edge.samples.end()) {
    return *(after - 1);
  }
  const auto before = after - 1;
  return (t - before->time) <= (after->time - t) ? *before : *after;
}

glm::mat4 TransformTree::toAncestor(
    const std::vector<std::pair<std::string_view, const Edge*>>& chain,
    std::string_view ancestor, Time t) const {
  glm::mat4 result(1.0F);
  for (const auto& [child, edge] : chain) {
    if (child == ancestor) {
      break;
    }
    result = toMatrix(nearest(*edge, t)) * result;
  }
  return result;
}

std::optional<glm::mat4> TransformTree::lookup(std::string_view target,
                                               std::string_view source,
                                               Time t) const {
  if (target == source) {
    return glm::mat4(1.0F);
  }
  const auto sourceChain = chain(source);
  const auto targetChain = chain(target);
  if (sourceChain.empty() && targetChain.empty()) {
    return std::nullopt;
  }

  // Frames on the source's path to its root, the root included.
  std::set<std::string_view> sourceAncestors{source};
  for (const auto& [child, edge] : sourceChain) {
    sourceAncestors.insert(edge->parent);
  }
  // The first frame on the target's path that the source's path also visits.
  std::string_view common;
  bool found = sourceAncestors.contains(target);
  if (found) {
    common = target;
  } else {
    for (const auto& [child, edge] : targetChain) {
      if (sourceAncestors.contains(edge->parent)) {
        common = edge->parent;
        found = true;
        break;
      }
    }
  }
  if (!found) {
    return std::nullopt;
  }
  const glm::mat4 sourceInCommon = toAncestor(sourceChain, common, t);
  const glm::mat4 targetInCommon = toAncestor(targetChain, common, t);
  return glm::inverse(targetInCommon) * sourceInCommon;
}

std::vector<std::string> TransformTree::frames() const {
  std::set<std::string> names;
  for (const auto& [child, edge] : m_edges) {
    names.insert(child);
    names.insert(edge.parent);
  }
  return {names.begin(), names.end()};
}

std::optional<std::string> TransformTree::parent(std::string_view child) const {
  const Edge* found = edge(child);
  if (found == nullptr) {
    return std::nullopt;
  }
  return found->parent;
}

size_t TransformTree::sampleCount(std::string_view child) const {
  const Edge* found = edge(child);
  return found == nullptr ? 0 : found->samples.size();
}

void TransformTree::clear() { m_edges.clear(); }

}  // namespace camelot

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

#ifndef CAMELOT_SRC_DATA_TRANSFORMTREE_H_
#define CAMELOT_SRC_DATA_TRANSFORMTREE_H_

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "Camelot/src/data/FoxgloveMessages.h"

namespace camelot {

// The frame hierarchy built from FrameTransform messages: every child frame
// has one parent and a time-ordered list of samples. Lookups walk the parent
// chain of both frames to their common ancestor and take the sample nearest
// in time on every edge, so an edge with a single sample (a static
// transform) always matches. Matrices are column-major GLM, right-handed,
// quaternions (x, y, z, w) as in the schema.
class TransformTree {
 public:
  struct Sample {
    Time time{0};
    Vec3 translation;
    Quat rotation;
  };

  // Records one transform; a child that changes parent drops its old samples.
  void add(const FrameTransform& transform);

  // The pose of `source` expressed in `target` at time `t` (a point in
  // `source` coordinates times the matrix is the point in `target`), or
  // nullopt when the frames are not connected.
  [[nodiscard]] std::optional<glm::mat4> lookup(std::string_view target,
                                                std::string_view source,
                                                Time t) const;

  // Every frame name seen so far (parents and children), sorted.
  [[nodiscard]] std::vector<std::string> frames() const;
  // The parent of `child`, nullopt for a root or unknown frame.
  [[nodiscard]] std::optional<std::string> parent(std::string_view child) const;
  // Number of samples stored for `child`'s edge (0 for an unknown child).
  [[nodiscard]] size_t sampleCount(std::string_view child) const;

  void clear();

  // The matrix of one sample (translate * rotate); a zero quaternion is
  // treated as identity.
  [[nodiscard]] static glm::mat4 toMatrix(const Sample& sample);

 private:
  struct Edge {
    std::string parent;
    std::vector<Sample> samples;  // sorted by time
  };

  // child frame -> edge to its parent
  std::map<std::string, Edge, std::less<>> m_edges;

  [[nodiscard]] const Edge* edge(std::string_view child) const;
  // The chain of (child, edge) from `frame` up to its root, root last.
  [[nodiscard]] std::vector<std::pair<std::string_view, const Edge*>> chain(
      std::string_view frame) const;
  // Transform of `frame` in the coordinates of its ancestor `ancestor`.
  [[nodiscard]] glm::mat4 toAncestor(
      const std::vector<std::pair<std::string_view, const Edge*>>& chain,
      std::string_view ancestor, Time t) const;
  [[nodiscard]] static const Sample& nearest(const Edge& edge, Time t);
};

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_TRANSFORMTREE_H_

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

#ifndef CAMELOT_SRC_DATA_FOXGLOVEDECODER_H_
#define CAMELOT_SRC_DATA_FOXGLOVEDECODER_H_

#include <cstddef>
#include <span>

#include "Camelot/src/data/FoxgloveMessages.h"

// Decoders from the protobuf wire format of the Foxglove schemas to the plain
// structs of FoxgloveMessages.h. Every decoder throws std::runtime_error
// prefixed with the schema name on malformed input; unknown fields are
// skipped so newer schema revisions still decode.

namespace camelot {

[[nodiscard]] PointCloud decodePointCloud(std::span<const std::byte> message);
[[nodiscard]] CompressedImage decodeCompressedImage(
    std::span<const std::byte> message);
[[nodiscard]] FrameTransform decodeFrameTransform(
    std::span<const std::byte> message);
[[nodiscard]] SceneUpdate decodeSceneUpdate(std::span<const std::byte> message);
[[nodiscard]] ImageAnnotations decodeImageAnnotations(
    std::span<const std::byte> message);
[[nodiscard]] CameraCalibration decodeCameraCalibration(
    std::span<const std::byte> message);

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_FOXGLOVEDECODER_H_

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

#ifndef CAMELOT_SRC_DATA_MCAPSOURCE_H_
#define CAMELOT_SRC_DATA_MCAPSOURCE_H_

#include <cstddef>
#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Camelot/src/data/FoxgloveMessages.h"

namespace camelot {

// One topic of the recording. Several channels may share a topic; their
// message counts are summed and the schema of the first one is reported.
struct TopicInfo {
  std::string topic;
  std::string schemaName;
  std::string encoding;
  uint64_t messageCount{0};
};

// A message as stored in the file. The views are valid only for the
// duration of the forEach() callback.
struct RawMessage {
  std::string_view topic;
  std::string_view schemaName;
  Time logTime{0};
  std::span<const std::byte> data;
};

// Read access to one MCAP file: the topic list, the time range, ordered
// iteration over a time window and random access to the latest message of a
// topic (for seeks). The mcap library and the compressed-chunk handling stay
// behind the pimpl so no user of this header needs the mcap headers.
class McapSource {
 public:
  McapSource();
  ~McapSource();
  McapSource(const McapSource&) = delete;
  McapSource& operator=(const McapSource&) = delete;
  McapSource(McapSource&&) noexcept;
  McapSource& operator=(McapSource&&) noexcept;

  // Opens the file and reads its summary. Throws std::runtime_error with the
  // mcap status text on failure; the source is then closed.
  void open(const std::filesystem::path& path);
  void close();
  [[nodiscard]] bool isOpen() const;
  [[nodiscard]] const std::filesystem::path& path() const;

  [[nodiscard]] const std::vector<TopicInfo>& topics() const;
  // Log time of the first and last message (both 0 for an empty file).
  [[nodiscard]] Time startTime() const;
  [[nodiscard]] Time endTime() const;
  [[nodiscard]] uint64_t messageCount() const;

  // Calls `onMessage` for every message with t0 <= logTime < t1 on the given
  // topics (empty = all), in log-time order.
  void forEach(Time t0, Time t1, std::span<const std::string> topics,
               const std::function<void(const RawMessage&)>& onMessage);

  // The payload of the last message with logTime <= t on `topic`, nullopt
  // when there is none. The per-topic time index is built from the file's
  // message indexes on first use (no chunk is decompressed for that) and a
  // few decompressed chunks are cached, so a seek that asks for every topic
  // touches each chunk once.
  [[nodiscard]] std::optional<std::vector<std::byte>> latestBefore(
      std::string_view topic, Time t);

 private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace camelot

#endif  // CAMELOT_SRC_DATA_MCAPSOURCE_H_

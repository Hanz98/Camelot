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

#include "Camelot/src/data/McapSource.h"

// The mcap library is header-only; this is the one translation unit that
// carries its implementation.
#define MCAP_IMPLEMENTATION
#include <algorithm>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <mcap/reader.hpp>

namespace camelot {

namespace {

// Decompressed chunks kept for latestBefore(); a seek asks for every topic
// and they mostly live in the same one or two chunks.
constexpr size_t kChunkCacheSize = 4;
constexpr size_t kRecordHeaderSize = 9;  // opcode + uint64 length

// Where one message lives: inside a chunk (offset relative to the start of
// the decompressed records) or, for an unchunked file, at an absolute file
// offset.
struct IndexEntry {
  Time logTime{0};
  std::optional<uint64_t> chunkStart;
  uint64_t offset{0};
};

struct CachedChunk {
  uint64_t chunkStart{0};
  std::vector<std::byte> records;
};

[[noreturn]] void fail(const std::string& what, const mcap::Status& status) {
  throw std::runtime_error("mcap: " + what + ": " + status.message);
}

void check(const mcap::Status& status, const std::string& what) {
  if (!status.ok()) {
    fail(what, status);
  }
}

uint64_t readUint64(std::span<const std::byte> at) {
  uint64_t value = 0;
  std::memcpy(&value, at.data(), sizeof(value));
  return value;
}

std::vector<std::byte> copyPayload(const mcap::Message& message) {
  const std::span<const std::byte> payload(message.data, message.dataSize);
  return {payload.begin(), payload.end()};
}

void sortByTime(std::vector<IndexEntry>& entries) {
  std::ranges::stable_sort(entries, {}, &IndexEntry::logTime);
}

}  // namespace

struct McapSource::Impl {
  mcap::McapReader reader;
  std::filesystem::path path;
  bool open{false};
  std::vector<TopicInfo> topics;
  std::map<std::string, std::vector<mcap::ChannelId>, std::less<>>
      channelsByTopic;
  Time startTime{0};
  Time endTime{0};
  uint64_t messageCount{0};
  bool hasChunkIndexes{false};
  bool hasMessageIndexes{false};

  // Per-topic time index, built on demand (see latestBefore()).
  std::map<std::string, std::vector<IndexEntry>, std::less<>> index;
  bool scanned{false};
  std::deque<CachedChunk> chunkCache;

  void readSummary();
  void collectTopics();
  const std::vector<IndexEntry>& indexFor(std::string_view topic);
  void buildFromMessageIndexes(std::string_view topic,
                               std::vector<IndexEntry>& entries);
  void buildByScanning();
  const std::vector<std::byte>& chunk(uint64_t chunkStart);
  std::vector<std::byte> messageAt(const IndexEntry& entry);
  std::vector<std::byte> messageInChunk(uint64_t chunkStart, uint64_t offset);
  std::vector<std::byte> messageInFile(uint64_t offset);
};

void McapSource::Impl::readSummary() {
  std::vector<mcap::Status> problems;
  check(reader.readSummary(
            mcap::ReadSummaryMethod::AllowFallbackScan,
            [&](const mcap::Status& status) { problems.push_back(status); }),
        "reading the summary of " + path.string());
  if (!problems.empty()) {
    fail("reading the summary of " + path.string(), problems.front());
  }
  const auto& chunkIndexes = reader.chunkIndexes();
  hasChunkIndexes = !chunkIndexes.empty();
  hasMessageIndexes = std::ranges::any_of(
      chunkIndexes,
      [](const mcap::ChunkIndex& c) { return !c.messageIndexOffsets.empty(); });
  if (const auto& stats = reader.statistics()) {
    startTime = stats->messageStartTime;
    endTime = stats->messageEndTime;
    messageCount = stats->messageCount;
  } else {
    for (const mcap::ChunkIndex& c : chunkIndexes) {
      startTime = std::min(startTime == 0 ? c.messageStartTime : startTime,
                           c.messageStartTime);
      endTime = std::max(endTime, c.messageEndTime);
    }
  }
}

void McapSource::Impl::collectTopics() {
  const auto channels = reader.channels();
  const auto& stats = reader.statistics();
  std::vector<mcap::ChannelId> ids;
  ids.reserve(channels.size());
  for (const auto& [id, channel] : channels) {
    ids.push_back(id);
  }
  std::ranges::sort(ids);
  for (const mcap::ChannelId id : ids) {
    const mcap::ChannelPtr& channel = channels.at(id);
    channelsByTopic[channel->topic].push_back(id);
    uint64_t count = 0;
    if (stats) {
      const auto found = stats->channelMessageCounts.find(id);
      count = found == stats->channelMessageCounts.end() ? 0 : found->second;
    }
    auto known = std::ranges::find_if(
        topics, [&](const TopicInfo& t) { return t.topic == channel->topic; });
    if (known != topics.end()) {
      known->messageCount += count;
      continue;
    }
    const mcap::SchemaPtr schema = reader.schema(channel->schemaId);
    topics.push_back({.topic = channel->topic,
                      .schemaName = schema ? schema->name : std::string(),
                      .encoding = channel->messageEncoding,
                      .messageCount = count});
  }
}

const std::vector<IndexEntry>& McapSource::Impl::indexFor(
    std::string_view topic) {
  const auto found = index.find(topic);
  if (found != index.end()) {
    return found->second;
  }
  std::vector<IndexEntry> entries;
  if (hasChunkIndexes && hasMessageIndexes) {
    buildFromMessageIndexes(topic, entries);
  } else if (!scanned) {
    // A file without indexes: one pass over everything fills every topic.
    buildByScanning();
    const auto built = index.find(topic);
    if (built != index.end()) {
      return built->second;
    }
  }
  sortByTime(entries);
  return index.emplace(std::string(topic), std::move(entries)).first->second;
}

void McapSource::Impl::buildFromMessageIndexes(
    std::string_view topic, std::vector<IndexEntry>& entries) {
  const auto channels = channelsByTopic.find(topic);
  if (channels == channelsByTopic.end()) {
    return;
  }
  for (const mcap::ChunkIndex& chunkIndex : reader.chunkIndexes()) {
    for (const mcap::ChannelId id : channels->second) {
      const auto offset = chunkIndex.messageIndexOffsets.find(id);
      if (offset == chunkIndex.messageIndexOffsets.end()) {
        continue;
      }
      mcap::Record record{};
      check(mcap::McapReader::ReadRecord(*reader.dataSource(), offset->second,
                                         &record),
            "reading a message index");
      if (record.opcode != mcap::OpCode::MessageIndex) {
        throw std::runtime_error(
            "mcap: message index offset does not point "
            "at a message index record");
      }
      mcap::MessageIndex messageIndex;
      check(mcap::McapReader::ParseMessageIndex(record, &messageIndex),
            "parsing a message index");
      for (const auto& [logTime, messageOffset] : messageIndex.records) {
        entries.push_back({.logTime = logTime,
                           .chunkStart = chunkIndex.chunkStartOffset,
                           .offset = messageOffset});
      }
    }
  }
}

void McapSource::Impl::buildByScanning() {
  scanned = true;
  std::vector<mcap::Status> problems;
  mcap::ReadMessageOptions options;
  options.readOrder = mcap::ReadMessageOptions::ReadOrder::FileOrder;
  for (const mcap::MessageView& view : reader.readMessages(
           [&](const mcap::Status& status) { problems.push_back(status); },
           options)) {
    index[view.channel->topic].push_back(
        {.logTime = view.message.logTime,
         .chunkStart = view.messageOffset.chunkOffset,
         .offset = view.messageOffset.offset});
  }
  if (!problems.empty()) {
    fail("scanning " + path.string(), problems.front());
  }
  for (auto& [topic, entries] : index) {
    sortByTime(entries);
  }
}

const std::vector<std::byte>& McapSource::Impl::chunk(uint64_t chunkStart) {
  for (const CachedChunk& cached : chunkCache) {
    if (cached.chunkStart == chunkStart) {
      return cached.records;
    }
  }
  mcap::Record record{};
  check(mcap::McapReader::ReadRecord(*reader.dataSource(), chunkStart, &record),
        "reading a chunk");
  if (record.opcode != mcap::OpCode::Chunk) {
    throw std::runtime_error(
        "mcap: chunk offset does not point at a chunk record");
  }
  mcap::Chunk chunk{};
  check(mcap::McapReader::ParseChunk(record, &chunk), "parsing a chunk");

  CachedChunk cached{.chunkStart = chunkStart, .records = {}};
  const auto compression =
      mcap::McapReader::ParseCompression(chunk.compression);
  if (!compression) {
    throw std::runtime_error("mcap: unrecognized chunk compression '" +
                             chunk.compression + "'");
  }
  switch (*compression) {
    case mcap::Compression::None: {
      const std::span<const std::byte> raw(chunk.records,
                                           chunk.uncompressedSize);
      cached.records.assign(raw.begin(), raw.end());
      break;
    }
    case mcap::Compression::Zstd:
      check(mcap::ZStdReader::DecompressAll(chunk.records, chunk.compressedSize,
                                            chunk.uncompressedSize,
                                            &cached.records),
            "decompressing a zstd chunk");
      break;
    case mcap::Compression::Lz4: {
      mcap::LZ4Reader lz4;
      check(lz4.decompressAll(chunk.records, chunk.compressedSize,
                              chunk.uncompressedSize, &cached.records),
            "decompressing an lz4 chunk");
      break;
    }
  }
  if (chunkCache.size() >= kChunkCacheSize) {
    chunkCache.pop_front();
  }
  chunkCache.push_back(std::move(cached));
  return chunkCache.back().records;
}

std::vector<std::byte> McapSource::Impl::messageInChunk(uint64_t chunkStart,
                                                        uint64_t offset) {
  const std::span<const std::byte> records(chunk(chunkStart));
  if (offset + kRecordHeaderSize > records.size()) {
    throw std::runtime_error("mcap: message offset beyond the chunk");
  }
  mcap::Record record{};
  record.opcode = static_cast<mcap::OpCode>(records[offset]);
  record.dataSize = readUint64(records.subspan(offset + 1));
  if (record.opcode != mcap::OpCode::Message ||
      offset + kRecordHeaderSize + record.dataSize > records.size()) {
    throw std::runtime_error("mcap: message index points at a bad record");
  }
  // mcap::Record carries a mutable pointer, but ParseMessage only reads.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  record.data = const_cast<std::byte*>(
      records.subspan(offset + kRecordHeaderSize).data());
  mcap::Message message{};
  check(mcap::McapReader::ParseMessage(record, &message), "parsing a message");
  return copyPayload(message);
}

std::vector<std::byte> McapSource::Impl::messageInFile(uint64_t offset) {
  mcap::Record record{};
  check(mcap::McapReader::ReadRecord(*reader.dataSource(), offset, &record),
        "reading a message");
  if (record.opcode != mcap::OpCode::Message) {
    throw std::runtime_error(
        "mcap: message offset does not point at a "
        "message record");
  }
  mcap::Message message{};
  check(mcap::McapReader::ParseMessage(record, &message), "parsing a message");
  return copyPayload(message);
}

std::vector<std::byte> McapSource::Impl::messageAt(const IndexEntry& entry) {
  if (entry.chunkStart.has_value()) {
    return messageInChunk(*entry.chunkStart, entry.offset);
  }
  return messageInFile(entry.offset);
}

McapSource::McapSource() : m_impl(std::make_unique<Impl>()) {}

McapSource::~McapSource() = default;

McapSource::McapSource(McapSource&&) noexcept = default;

McapSource& McapSource::operator=(McapSource&&) noexcept = default;

void McapSource::open(const std::filesystem::path& path) {
  close();
  m_impl->path = path;
  const mcap::Status status = m_impl->reader.open(path.string());
  if (!status.ok()) {
    fail("opening " + path.string(), status);
  }
  try {
    m_impl->readSummary();
    m_impl->collectTopics();
  } catch (...) {
    close();
    throw;
  }
  m_impl->open = true;
}

void McapSource::close() {
  m_impl->reader.close();
  m_impl = std::make_unique<Impl>();
}

bool McapSource::isOpen() const { return m_impl->open; }

const std::filesystem::path& McapSource::path() const { return m_impl->path; }

const std::vector<TopicInfo>& McapSource::topics() const {
  return m_impl->topics;
}

Time McapSource::startTime() const { return m_impl->startTime; }

Time McapSource::endTime() const { return m_impl->endTime; }

uint64_t McapSource::messageCount() const { return m_impl->messageCount; }

void McapSource::forEach(
    Time t0, Time t1, std::span<const std::string> topics,
    const std::function<void(const RawMessage&)>& onMessage) {
  if (!isOpen()) {
    throw std::runtime_error("mcap: no file is open");
  }
  if (t0 >= t1) {
    return;
  }
  mcap::ReadMessageOptions options(t0, t1);
  // Time-ordered reading needs chunk indexes; a file without them is read
  // as written, which is the writer's log order in practice.
  options.readOrder = m_impl->hasChunkIndexes && m_impl->hasMessageIndexes
                          ? mcap::ReadMessageOptions::ReadOrder::LogTimeOrder
                          : mcap::ReadMessageOptions::ReadOrder::FileOrder;
  const std::set<std::string_view> wanted(topics.begin(), topics.end());
  if (!wanted.empty()) {
    options.topicFilter = [&wanted](std::string_view topic) {
      return wanted.contains(topic);
    };
  }
  std::vector<mcap::Status> problems;
  for (const mcap::MessageView& view : m_impl->reader.readMessages(
           [&](const mcap::Status& status) { problems.push_back(status); },
           options)) {
    RawMessage raw;
    raw.topic = view.channel->topic;
    raw.schemaName =
        view.schema ? std::string_view(view.schema->name) : std::string_view();
    raw.logTime = view.message.logTime;
    raw.data = {view.message.data, view.message.dataSize};
    onMessage(raw);
  }
  if (!problems.empty()) {
    fail("reading " + m_impl->path.string(), problems.front());
  }
}

std::optional<std::vector<std::byte>> McapSource::latestBefore(
    std::string_view topic, Time t) {
  if (!isOpen()) {
    throw std::runtime_error("mcap: no file is open");
  }
  const std::vector<IndexEntry>& entries = m_impl->indexFor(topic);
  // The first entry after t; the one before it is the answer.
  const auto after =
      std::ranges::upper_bound(entries, t, {}, &IndexEntry::logTime);
  if (after == entries.begin()) {
    return std::nullopt;
  }
  return m_impl->messageAt(*(after - 1));
}

}  // namespace camelot

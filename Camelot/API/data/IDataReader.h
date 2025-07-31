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

#ifndef CAMELOT_API_DATA_IDATAREADER_H_
#define CAMELOT_API_DATA_IDATAREADER_H_

#include "DataTypes.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace camelot {
namespace data {

// Filter function types
using TopicFilter = std::function<bool(const std::string&)>;
using TimestampFilter = std::function<bool(const Timestamp&)>;

// Result enumeration for operations
enum class ReadResult {
    Success,
    FileNotFound,
    InvalidFormat,
    TopicNotFound,
    DeserializationError,
    UnknownError
};

// Abstract interface for data readers
class IDataReader {
public:
    virtual ~IDataReader() = default;

    // File operations
    virtual ReadResult open(const std::string& file_path) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;

    // Topic information
    virtual std::vector<std::string> get_topics() const = 0;
    virtual std::vector<std::string> get_topics_by_type(const std::string& message_type) const = 0;

    // Read all messages of specific types
    virtual ReadResult read_objects(std::vector<TimestampedMessage<Object>>& messages,
                                  const TopicFilter& topic_filter = nullptr) = 0;
    virtual ReadResult read_markers(std::vector<TimestampedMessage<Marker>>& messages,
                                  const TopicFilter& topic_filter = nullptr) = 0;
    virtual ReadResult read_lanes(std::vector<TimestampedMessage<Lane>>& messages,
                                const TopicFilter& topic_filter = nullptr) = 0;
    virtual ReadResult read_pointclouds(std::vector<TimestampedMessage<PointCloud>>& messages,
                                      const TopicFilter& topic_filter = nullptr) = 0;

    // Read messages in timestamp order
    virtual ReadResult read_all_messages_in_order(
        std::vector<TimestampedMessage<Object>>& objects,
        std::vector<TimestampedMessage<Marker>>& markers,
        std::vector<TimestampedMessage<Lane>>& lanes,
        std::vector<TimestampedMessage<PointCloud>>& pointclouds,
        const TimestampFilter& time_filter = nullptr) = 0;

    // Utility methods
    virtual Timestamp get_start_time() const = 0;
    virtual Timestamp get_end_time() const = 0;
    virtual size_t get_message_count() const = 0;
    virtual size_t get_message_count_by_topic(const std::string& topic) const = 0;

    // Error handling
    virtual std::string get_last_error() const = 0;
};

// Factory function type
using DataReaderFactory = std::function<std::unique_ptr<IDataReader>()>;

// Helper functions for creating filters
namespace filters {

// Create topic filter that matches specific topics
inline TopicFilter topics(const std::vector<std::string>& topic_names) {
    return [topic_names](const std::string& topic) {
        return std::find(topic_names.begin(), topic_names.end(), topic) != topic_names.end();
    };
}

// Create topic filter that matches topics containing a substring
inline TopicFilter topic_contains(const std::string& substring) {
    return [substring](const std::string& topic) {
        return topic.find(substring) != std::string::npos;
    };
}

// Create timestamp filter for a time range
inline TimestampFilter time_range(const Timestamp& start, const Timestamp& end) {
    return [start, end](const Timestamp& ts) {
        return ts >= start && ts <= end;
    };
}

// Create timestamp filter for messages after a specific time
inline TimestampFilter time_after(const Timestamp& start) {
    return [start](const Timestamp& ts) {
        return ts >= start;
    };
}

// Create timestamp filter for messages before a specific time
inline TimestampFilter time_before(const Timestamp& end) {
    return [end](const Timestamp& ts) {
        return ts <= end;
    };
}

} // namespace filters

} // namespace data
} // namespace camelot

#endif // CAMELOT_API_DATA_IDATAREADER_H_
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

#ifndef CAMELOT_SRC_DATA_MCAPREADER_H_
#define CAMELOT_SRC_DATA_MCAPREADER_H_

#include <Camelot/API/data/IDataReader.h>
#include <mcap/reader.hpp>
#include <fstream>
#include <memory>
#include <unordered_map>

namespace camelot {
namespace data {

class McapReader : public IDataReader {
private:
    std::ifstream file_stream_;
    std::unique_ptr<mcap::McapReader> reader_;
    std::string file_path_;
    std::string last_error_;
    bool is_open_;
    
    // Cached information about the file
    std::vector<std::string> topics_;
    std::unordered_map<std::string, std::string> topic_to_type_;
    std::unordered_map<std::string, size_t> topic_message_counts_;
    Timestamp start_time_;
    Timestamp end_time_;
    size_t total_message_count_;

    // Message type constants
    static constexpr const char* OBJECT_TYPE = "camelot/Object";
    static constexpr const char* MARKER_TYPE = "camelot/Marker";
    static constexpr const char* LANE_TYPE = "camelot/Lane";
    static constexpr const char* POINTCLOUD_TYPE = "camelot/PointCloud";

    // Helper methods
    ReadResult scan_file_info();
    Timestamp mcap_time_to_timestamp(uint64_t mcap_time) const;
    uint64_t timestamp_to_mcap_time(const Timestamp& ts) const;
    
    // Deserialization methods
    ReadResult deserialize_object(const std::string& data, Object& object) const;
    ReadResult deserialize_marker(const std::string& data, Marker& marker) const;
    ReadResult deserialize_lane(const std::string& data, Lane& lane) const;
    ReadResult deserialize_pointcloud(const std::string& data, PointCloud& pointcloud) const;

    // Template method for reading messages of specific type
    template<typename T>
    ReadResult read_messages_of_type(const std::string& message_type,
                                   std::vector<TimestampedMessage<T>>& messages,
                                   const TopicFilter& topic_filter,
                                   ReadResult (McapReader::*deserializer)(const std::string&, T&) const);

public:
    McapReader();
    ~McapReader() override;

    // IDataReader implementation
    ReadResult open(const std::string& file_path) override;
    void close() override;
    bool is_open() const override;

    std::vector<std::string> get_topics() const override;
    std::vector<std::string> get_topics_by_type(const std::string& message_type) const override;

    ReadResult read_objects(std::vector<TimestampedMessage<Object>>& messages,
                          const TopicFilter& topic_filter = nullptr) override;
    ReadResult read_markers(std::vector<TimestampedMessage<Marker>>& messages,
                          const TopicFilter& topic_filter = nullptr) override;
    ReadResult read_lanes(std::vector<TimestampedMessage<Lane>>& messages,
                        const TopicFilter& topic_filter = nullptr) override;
    ReadResult read_pointclouds(std::vector<TimestampedMessage<PointCloud>>& messages,
                              const TopicFilter& topic_filter = nullptr) override;

    ReadResult read_all_messages_in_order(
        std::vector<TimestampedMessage<Object>>& objects,
        std::vector<TimestampedMessage<Marker>>& markers,
        std::vector<TimestampedMessage<Lane>>& lanes,
        std::vector<TimestampedMessage<PointCloud>>& pointclouds,
        const TimestampFilter& time_filter = nullptr) override;

    Timestamp get_start_time() const override;
    Timestamp get_end_time() const override;
    size_t get_message_count() const override;
    size_t get_message_count_by_topic(const std::string& topic) const override;

    std::string get_last_error() const override;
};

// Factory function for creating MCAP readers
std::unique_ptr<IDataReader> create_mcap_reader();

} // namespace data
} // namespace camelot

#endif // CAMELOT_SRC_DATA_MCAPREADER_H_
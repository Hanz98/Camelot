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

#include "McapReader.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace pendragon {
namespace data {

McapReader::McapReader()
    : reader_(nullptr), is_open_(false), total_message_count_(0) {
}

McapReader::~McapReader() {
    close();
}

ReadResult McapReader::open(const std::string& file_path) {
    close(); // Close any previously opened file

    file_path_ = file_path;
    file_stream_.open(file_path, std::ios::binary);
    
    if (!file_stream_.is_open()) {
        last_error_ = "Failed to open file: " + file_path;
        return ReadResult::FileNotFound;
    }

    try {
        reader_ = std::make_unique<mcap::McapReader>();
        auto status = reader_->open(file_stream_);
        
        if (!status.ok()) {
            last_error_ = "Failed to open MCAP file: " + status.message;
            file_stream_.close();
            reader_.reset();
            return ReadResult::InvalidFormat;
        }

        is_open_ = true;
        auto scan_result = scan_file_info();
        if (scan_result != ReadResult::Success) {
            close();
            return scan_result;
        }

        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Exception while opening MCAP file: " + std::string(e.what());
        file_stream_.close();
        reader_.reset();
        return ReadResult::UnknownError;
    }
}

void McapReader::close() {
    if (reader_) {
        reader_->close();
        reader_.reset();
    }
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    is_open_ = false;
    topics_.clear();
    topic_to_type_.clear();
    topic_message_counts_.clear();
    total_message_count_ = 0;
}

bool McapReader::is_open() const {
    return is_open_;
}

ReadResult McapReader::scan_file_info() {
    if (!reader_) {
        last_error_ = "Reader not initialized";
        return ReadResult::UnknownError;
    }

    try {
        topics_.clear();
        topic_to_type_.clear();
        topic_message_counts_.clear();
        total_message_count_ = 0;
        start_time_ = Timestamp::max();
        end_time_ = Timestamp::min();

        // Read summary information
        auto summary = reader_->readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);
        if (!summary) {
            last_error_ = "Failed to read MCAP summary";
            return ReadResult::InvalidFormat;
        }

        // Extract topic information
        for (const auto& [channel_id, channel] : summary->channels) {
            const std::string& topic = channel->topic;
            topics_.push_back(topic);
            topic_to_type_[topic] = channel->schemaId ? 
                reader_->getSchema(channel->schemaId)->name : "unknown";
            topic_message_counts_[topic] = 0;
        }

        // Extract timing information and count messages
        for (const auto& [channel_id, stats] : summary->statistics) {
            auto channel_it = summary->channels.find(channel_id);
            if (channel_it != summary->channels.end()) {
                const std::string& topic = channel_it->second->topic;
                topic_message_counts_[topic] = stats->messageCount;
                total_message_count_ += stats->messageCount;
                
                if (stats->messageStartTime < timestamp_to_mcap_time(start_time_)) {
                    start_time_ = mcap_time_to_timestamp(stats->messageStartTime);
                }
                if (stats->messageEndTime > timestamp_to_mcap_time(end_time_)) {
                    end_time_ = mcap_time_to_timestamp(stats->messageEndTime);
                }
            }
        }

        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Exception while scanning file info: " + std::string(e.what());
        return ReadResult::UnknownError;
    }
}

Timestamp McapReader::mcap_time_to_timestamp(uint64_t mcap_time) const {
    return Timestamp(mcap_time);
}

uint64_t McapReader::timestamp_to_mcap_time(const Timestamp& ts) const {
    return static_cast<uint64_t>(ts.count());
}

std::vector<std::string> McapReader::get_topics() const {
    return topics_;
}

std::vector<std::string> McapReader::get_topics_by_type(const std::string& message_type) const {
    std::vector<std::string> filtered_topics;
    for (const auto& [topic, type] : topic_to_type_) {
        if (type == message_type) {
            filtered_topics.push_back(topic);
        }
    }
    return filtered_topics;
}

ReadResult McapReader::read_objects(std::vector<TimestampedMessage<Object>>& messages,
                                  const TopicFilter& topic_filter) {
    return read_messages_of_type<Object>(OBJECT_TYPE, messages, topic_filter,
                                       &McapReader::deserialize_object);
}

ReadResult McapReader::read_markers(std::vector<TimestampedMessage<Marker>>& messages,
                                  const TopicFilter& topic_filter) {
    return read_messages_of_type<Marker>(MARKER_TYPE, messages, topic_filter,
                                       &McapReader::deserialize_marker);
}

ReadResult McapReader::read_lanes(std::vector<TimestampedMessage<Lane>>& messages,
                                const TopicFilter& topic_filter) {
    return read_messages_of_type<Lane>(LANE_TYPE, messages, topic_filter,
                                     &McapReader::deserialize_lane);
}

ReadResult McapReader::read_pointclouds(std::vector<TimestampedMessage<PointCloud>>& messages,
                                      const TopicFilter& topic_filter) {
    return read_messages_of_type<PointCloud>(POINTCLOUD_TYPE, messages, topic_filter,
                                           &McapReader::deserialize_pointcloud);
}

template<typename T>
ReadResult McapReader::read_messages_of_type(const std::string& message_type,
                                           std::vector<TimestampedMessage<T>>& messages,
                                           const TopicFilter& topic_filter,
                                           ReadResult (McapReader::*deserializer)(const std::string&, T&) const) {
    if (!is_open_) {
        last_error_ = "File not open";
        return ReadResult::UnknownError;
    }

    try {
        messages.clear();
        
        auto relevant_topics = get_topics_by_type(message_type);
        if (relevant_topics.empty()) {
            return ReadResult::TopicNotFound;
        }

        // Filter topics if filter is provided
        if (topic_filter) {
            relevant_topics.erase(
                std::remove_if(relevant_topics.begin(), relevant_topics.end(),
                             [&topic_filter](const std::string& topic) {
                                 return !topic_filter(topic);
                             }),
                relevant_topics.end());
        }

        if (relevant_topics.empty()) {
            return ReadResult::TopicNotFound;
        }

        // Read messages from relevant topics
        auto message_view = reader_->readMessages();
        for (const auto& message_ptr : message_view) {
            const auto& message = *message_ptr;
            
            // Check if this message is from a relevant topic
            auto channel = reader_->getChannel(message.channelId);
            if (!channel) continue;
            
            const std::string& topic = channel->topic;
            if (std::find(relevant_topics.begin(), relevant_topics.end(), topic) == relevant_topics.end()) {
                continue;
            }

            // Deserialize the message
            T data;
            std::string message_data(reinterpret_cast<const char*>(message.data), message.dataSize);
            auto result = (this->*deserializer)(message_data, data);
            
            if (result == ReadResult::Success) {
                messages.emplace_back(mcap_time_to_timestamp(message.logTime), topic, data);
            }
        }

        // Sort messages by timestamp
        std::sort(messages.begin(), messages.end(),
                  [](const TimestampedMessage<T>& a, const TimestampedMessage<T>& b) {
                      return a.timestamp < b.timestamp;
                  });

        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Exception while reading messages: " + std::string(e.what());
        return ReadResult::UnknownError;
    }
}

ReadResult McapReader::read_all_messages_in_order(
    std::vector<TimestampedMessage<Object>>& objects,
    std::vector<TimestampedMessage<Marker>>& markers,
    std::vector<TimestampedMessage<Lane>>& lanes,
    std::vector<TimestampedMessage<PointCloud>>& pointclouds,
    const TimestampFilter& time_filter) {
    
    // Read all message types
    auto obj_result = read_objects(objects);
    auto marker_result = read_markers(markers);
    auto lane_result = read_lanes(lanes);
    auto pc_result = read_pointclouds(pointclouds);

    // Apply time filter if provided
    if (time_filter) {
        objects.erase(std::remove_if(objects.begin(), objects.end(),
                                   [&time_filter](const TimestampedMessage<Object>& msg) {
                                       return !time_filter(msg.timestamp);
                                   }), objects.end());
        
        markers.erase(std::remove_if(markers.begin(), markers.end(),
                                   [&time_filter](const TimestampedMessage<Marker>& msg) {
                                       return !time_filter(msg.timestamp);
                                   }), markers.end());
        
        lanes.erase(std::remove_if(lanes.begin(), lanes.end(),
                                 [&time_filter](const TimestampedMessage<Lane>& msg) {
                                     return !time_filter(msg.timestamp);
                                 }), lanes.end());
        
        pointclouds.erase(std::remove_if(pointclouds.begin(), pointclouds.end(),
                                       [&time_filter](const TimestampedMessage<PointCloud>& msg) {
                                           return !time_filter(msg.timestamp);
                                       }), pointclouds.end());
    }

    // Return success if at least one type was successfully read
    if (obj_result == ReadResult::Success || marker_result == ReadResult::Success ||
        lane_result == ReadResult::Success || pc_result == ReadResult::Success) {
        return ReadResult::Success;
    }

    return ReadResult::TopicNotFound;
}

Timestamp McapReader::get_start_time() const {
    return start_time_;
}

Timestamp McapReader::get_end_time() const {
    return end_time_;
}

size_t McapReader::get_message_count() const {
    return total_message_count_;
}

size_t McapReader::get_message_count_by_topic(const std::string& topic) const {
    auto it = topic_message_counts_.find(topic);
    return (it != topic_message_counts_.end()) ? it->second : 0;
}

std::string McapReader::get_last_error() const {
    return last_error_;
}

// Basic deserialization methods (simplified for initial implementation)
// In a real implementation, these would use proper serialization format (e.g., protobuf, msgpack)
ReadResult McapReader::deserialize_object(const std::string& data, Object& object) const {
    try {
        // Simplified JSON-like parsing for demonstration
        // In practice, this would use a proper serialization library
        std::istringstream ss(data);
        std::string line;
        
        // Parse basic fields - this is a simplified example
        while (std::getline(ss, line)) {
            if (line.find("id:") != std::string::npos) {
                object.id = line.substr(line.find(":") + 1);
            } else if (line.find("type:") != std::string::npos) {
                object.type = line.substr(line.find(":") + 1);
            }
            // Add more parsing as needed
        }
        
        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Failed to deserialize object: " + std::string(e.what());
        return ReadResult::DeserializationError;
    }
}

ReadResult McapReader::deserialize_marker(const std::string& data, Marker& marker) const {
    try {
        // Simplified deserialization
        std::istringstream ss(data);
        std::string line;
        
        while (std::getline(ss, line)) {
            if (line.find("id:") != std::string::npos) {
                marker.id = line.substr(line.find(":") + 1);
            } else if (line.find("type:") != std::string::npos) {
                marker.type = line.substr(line.find(":") + 1);
            }
        }
        
        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Failed to deserialize marker: " + std::string(e.what());
        return ReadResult::DeserializationError;
    }
}

ReadResult McapReader::deserialize_lane(const std::string& data, Lane& lane) const {
    try {
        // Simplified deserialization
        std::istringstream ss(data);
        std::string line;
        
        while (std::getline(ss, line)) {
            if (line.find("id:") != std::string::npos) {
                lane.id = line.substr(line.find(":") + 1);
            } else if (line.find("direction:") != std::string::npos) {
                lane.direction = line.substr(line.find(":") + 1);
            }
        }
        
        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Failed to deserialize lane: " + std::string(e.what());
        return ReadResult::DeserializationError;
    }
}

ReadResult McapReader::deserialize_pointcloud(const std::string& data, PointCloud& pointcloud) const {
    try {
        // Simplified deserialization
        std::istringstream ss(data);
        std::string line;
        
        while (std::getline(ss, line)) {
            if (line.find("frame_id:") != std::string::npos) {
                pointcloud.frame_id = line.substr(line.find(":") + 1);
            }
        }
        
        return ReadResult::Success;
    } catch (const std::exception& e) {
        last_error_ = "Failed to deserialize pointcloud: " + std::string(e.what());
        return ReadResult::DeserializationError;
    }
}

// Factory function
std::unique_ptr<IDataReader> create_mcap_reader() {
    return std::make_unique<McapReader>();
}

} // namespace data
} // namespace pendragon
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

/**
 * Example usage of the MCAP reader for reading various data types
 * 
 * This example demonstrates how to:
 * - Open an MCAP file
 * - Read different message types (Objects, Markers, Lanes, PointClouds)
 * - Apply filters
 * - Handle errors
 */

#include <Pendragon/src/data/McapReader.h>
#include <Pendragon/API/data/DataTypes.h>
#include <iostream>
#include <iomanip>

using namespace pendragon::data;

void print_timestamp(const Timestamp& ts) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ts);
    auto nanoseconds = ts - seconds;
    std::cout << seconds.count() << "." << std::setfill('0') << std::setw(9) << nanoseconds.count();
}

void print_file_info(const IDataReader& reader) {
    std::cout << "\n=== File Information ===" << std::endl;
    std::cout << "Total messages: " << reader.get_message_count() << std::endl;
    std::cout << "Start time: ";
    print_timestamp(reader.get_start_time());
    std::cout << " seconds" << std::endl;
    std::cout << "End time: ";
    print_timestamp(reader.get_end_time());
    std::cout << " seconds" << std::endl;
    
    auto topics = reader.get_topics();
    std::cout << "Topics (" << topics.size() << "):" << std::endl;
    for (const auto& topic : topics) {
        std::cout << "  - " << topic << " (" << reader.get_message_count_by_topic(topic) << " messages)" << std::endl;
    }
}

void print_objects(const std::vector<TimestampedMessage<Object>>& objects) {
    std::cout << "\n=== Objects (" << objects.size() << ") ===" << std::endl;
    for (const auto& msg : objects) {
        const auto& obj = msg.data;
        std::cout << "Object [" << msg.topic << "] @ ";
        print_timestamp(msg.timestamp);
        std::cout << "s: ID=" << obj.id << ", Type=" << obj.type 
                  << ", Pos=(" << obj.pose.position.x << "," << obj.pose.position.y << "," << obj.pose.position.z << ")"
                  << ", Confidence=" << obj.confidence << std::endl;
    }
}

void print_markers(const std::vector<TimestampedMessage<Marker>>& markers) {
    std::cout << "\n=== Markers (" << markers.size() << ") ===" << std::endl;
    for (const auto& msg : markers) {
        const auto& marker = msg.data;
        std::cout << "Marker [" << msg.topic << "] @ ";
        print_timestamp(msg.timestamp);
        std::cout << "s: ID=" << marker.id << ", Type=" << marker.type 
                  << ", Pos=(" << marker.pose.position.x << "," << marker.pose.position.y << "," << marker.pose.position.z << ")"
                  << ", Text=" << marker.text << std::endl;
    }
}

void print_lanes(const std::vector<TimestampedMessage<Lane>>& lanes) {
    std::cout << "\n=== Lanes (" << lanes.size() << ") ===" << std::endl;
    for (const auto& msg : lanes) {
        const auto& lane = msg.data;
        std::cout << "Lane [" << msg.topic << "] @ ";
        print_timestamp(msg.timestamp);
        std::cout << "s: ID=" << lane.id << ", Direction=" << lane.direction 
                  << ", Points=" << lane.points.size() << ", Speed Limit=" << lane.speed_limit << std::endl;
    }
}

void print_pointclouds(const std::vector<TimestampedMessage<PointCloud>>& pointclouds) {
    std::cout << "\n=== PointClouds (" << pointclouds.size() << ") ===" << std::endl;
    for (const auto& msg : pointclouds) {
        const auto& pc = msg.data;
        std::cout << "PointCloud [" << msg.topic << "] @ ";
        print_timestamp(msg.timestamp);
        std::cout << "s: Frame=" << pc.frame_id << ", Points=" << pc.points.size() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mcap_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    
    // Create MCAP reader
    auto reader = create_mcap_reader();
    
    // Open the file
    std::cout << "Opening MCAP file: " << filename << std::endl;
    auto result = reader->open(filename);
    
    if (result != ReadResult::Success) {
        std::cerr << "Failed to open file: " << reader->get_last_error() << std::endl;
        return 1;
    }
    
    // Print file information
    print_file_info(*reader);
    
    // Example 1: Read all objects
    std::cout << "\n--- Example 1: Reading all objects ---" << std::endl;
    std::vector<TimestampedMessage<Object>> objects;
    result = reader->read_objects(objects);
    if (result == ReadResult::Success) {
        print_objects(objects);
    } else {
        std::cout << "No objects found or error reading objects" << std::endl;
    }
    
    // Example 2: Read markers with topic filter
    std::cout << "\n--- Example 2: Reading markers from specific topics ---" << std::endl;
    std::vector<TimestampedMessage<Marker>> markers;
    auto topic_filter = filters::topic_contains("marker");
    result = reader->read_markers(markers, topic_filter);
    if (result == ReadResult::Success) {
        print_markers(markers);
    } else {
        std::cout << "No markers found with topic filter" << std::endl;
    }
    
    // Example 3: Read lanes
    std::cout << "\n--- Example 3: Reading all lanes ---" << std::endl;
    std::vector<TimestampedMessage<Lane>> lanes;
    result = reader->read_lanes(lanes);
    if (result == ReadResult::Success) {
        print_lanes(lanes);
    } else {
        std::cout << "No lanes found or error reading lanes" << std::endl;
    }
    
    // Example 4: Read pointclouds
    std::cout << "\n--- Example 4: Reading all pointclouds ---" << std::endl;
    std::vector<TimestampedMessage<PointCloud>> pointclouds;
    result = reader->read_pointclouds(pointclouds);
    if (result == ReadResult::Success) {
        print_pointclouds(pointclouds);
    } else {
        std::cout << "No pointclouds found or error reading pointclouds" << std::endl;
    }
    
    // Example 5: Read all messages in temporal order with time filter
    std::cout << "\n--- Example 5: Reading all messages in temporal order ---" << std::endl;
    objects.clear();
    markers.clear();
    lanes.clear();
    pointclouds.clear();
    
    // Create a time filter for first half of the data
    auto start_time = reader->get_start_time();
    auto end_time = reader->get_end_time();
    auto mid_time = Timestamp((start_time.count() + end_time.count()) / 2);
    auto time_filter = filters::time_range(start_time, mid_time);
    
    result = reader->read_all_messages_in_order(objects, markers, lanes, pointclouds, time_filter);
    if (result == ReadResult::Success) {
        std::cout << "Filtered data (first half of timeline):" << std::endl;
        std::cout << "  Objects: " << objects.size() << std::endl;
        std::cout << "  Markers: " << markers.size() << std::endl;
        std::cout << "  Lanes: " << lanes.size() << std::endl;
        std::cout << "  PointClouds: " << pointclouds.size() << std::endl;
    } else {
        std::cout << "Error reading messages with time filter" << std::endl;
    }
    
    // Example 6: Topic filtering by specific topics
    std::cout << "\n--- Example 6: Topic-specific filtering ---" << std::endl;
    auto object_topics = reader->get_topics_by_type("camelot/Object");
    std::cout << "Object topics: ";
    for (const auto& topic : object_topics) {
        std::cout << topic << " ";
    }
    std::cout << std::endl;
    
    auto marker_topics = reader->get_topics_by_type("camelot/Marker");
    std::cout << "Marker topics: ";
    for (const auto& topic : marker_topics) {
        std::cout << topic << " ";
    }
    std::cout << std::endl;
    
    // Close the file
    reader->close();
    
    std::cout << "\nExample completed successfully!" << std::endl;
    return 0;
}
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
 * CLI tool to print summaries from .mcap files
 * 
 * Usage: mcap_summary <mcap_file> [options]
 * 
 * Options:
 *   --topics        Show detailed topic information
 *   --objects       Show object message summaries
 *   --markers       Show marker message summaries  
 *   --lanes         Show lane message summaries
 *   --pointclouds   Show pointcloud message summaries
 *   --all           Show all available information
 */

#include <Pendragon/src/data/McapReader.h>
#include <Pendragon/API/data/DataTypes.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace pendragon::data;

struct Options {
    bool show_topics = false;
    bool show_objects = false;
    bool show_markers = false;
    bool show_lanes = false;
    bool show_pointclouds = false;
    bool show_all = false;
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <mcap_file> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --topics        Show detailed topic information\n";
    std::cout << "  --objects       Show object message summaries\n";
    std::cout << "  --markers       Show marker message summaries\n";
    std::cout << "  --lanes         Show lane message summaries\n";
    std::cout << "  --pointclouds   Show pointcloud message summaries\n";
    std::cout << "  --all           Show all available information\n";
    std::cout << "  --help, -h      Show this help message\n";
}

Options parse_options(int argc, char* argv[]) {
    Options opts;
    
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--topics") {
            opts.show_topics = true;
        } else if (arg == "--objects") {
            opts.show_objects = true;
        } else if (arg == "--markers") {
            opts.show_markers = true;
        } else if (arg == "--lanes") {
            opts.show_lanes = true;
        } else if (arg == "--pointclouds") {
            opts.show_pointclouds = true;
        } else if (arg == "--all") {
            opts.show_all = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
        }
    }
    
    return opts;
}

void print_timestamp(const Timestamp& ts) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ts);
    auto nanoseconds = ts - seconds;
    std::cout << seconds.count() << "." << std::setfill('0') << std::setw(9) << nanoseconds.count();
}

void print_duration(const Timestamp& duration) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration - seconds);
    std::cout << seconds.count() << "." << std::setfill('0') << std::setw(3) << milliseconds.count();
}

void print_basic_info(const IDataReader& reader) {
    std::cout << "=== MCAP File Summary ===" << std::endl;
    std::cout << "Total messages: " << reader.get_message_count() << std::endl;
    std::cout << "Total topics: " << reader.get_topics().size() << std::endl;
    std::cout << "Start time: ";
    print_timestamp(reader.get_start_time());
    std::cout << "s" << std::endl;
    std::cout << "End time: ";
    print_timestamp(reader.get_end_time());
    std::cout << "s" << std::endl;
    std::cout << "Duration: ";
    print_duration(reader.get_end_time() - reader.get_start_time());
    std::cout << "s" << std::endl;
}

void print_topics_info(const IDataReader& reader) {
    std::cout << "\n=== Topics ===" << std::endl;
    auto topics = reader.get_topics();
    
    for (const auto& topic : topics) {
        auto count = reader.get_message_count_by_topic(topic);
        std::cout << std::setw(50) << std::left << topic << " " 
                  << std::setw(8) << std::right << count << " messages" << std::endl;
    }
    
    // Show breakdown by message type
    std::cout << "\n=== Message Types ===" << std::endl;
    auto object_topics = reader.get_topics_by_type("camelot/Object");
    auto marker_topics = reader.get_topics_by_type("camelot/Marker");
    auto lane_topics = reader.get_topics_by_type("camelot/Lane");
    auto pc_topics = reader.get_topics_by_type("camelot/PointCloud");
    
    std::cout << "Objects: " << object_topics.size() << " topics" << std::endl;
    std::cout << "Markers: " << marker_topics.size() << " topics" << std::endl;
    std::cout << "Lanes: " << lane_topics.size() << " topics" << std::endl;
    std::cout << "PointClouds: " << pc_topics.size() << " topics" << std::endl;
}

void print_objects_summary(IDataReader& reader) {
    std::cout << "\n=== Objects Summary ===" << std::endl;
    
    std::vector<TimestampedMessage<Object>> objects;
    auto result = reader.read_objects(objects);
    
    if (result != ReadResult::Success) {
        std::cout << "No objects found or error reading objects" << std::endl;
        return;
    }
    
    std::cout << "Total objects: " << objects.size() << std::endl;
    
    // Count by type
    std::map<std::string, int> type_counts;
    for (const auto& msg : objects) {
        type_counts[msg.data.type]++;
    }
    
    std::cout << "Object types:" << std::endl;
    for (const auto& [type, count] : type_counts) {
        std::cout << "  " << std::setw(20) << std::left << type << " " << count << std::endl;
    }
}

void print_markers_summary(IDataReader& reader) {
    std::cout << "\n=== Markers Summary ===" << std::endl;
    
    std::vector<TimestampedMessage<Marker>> markers;
    auto result = reader.read_markers(markers);
    
    if (result != ReadResult::Success) {
        std::cout << "No markers found or error reading markers" << std::endl;
        return;
    }
    
    std::cout << "Total markers: " << markers.size() << std::endl;
    
    // Count by type
    std::map<std::string, int> type_counts;
    for (const auto& msg : markers) {
        type_counts[msg.data.type]++;
    }
    
    std::cout << "Marker types:" << std::endl;
    for (const auto& [type, count] : type_counts) {
        std::cout << "  " << std::setw(20) << std::left << type << " " << count << std::endl;
    }
}

void print_lanes_summary(IDataReader& reader) {
    std::cout << "\n=== Lanes Summary ===" << std::endl;
    
    std::vector<TimestampedMessage<Lane>> lanes;
    auto result = reader.read_lanes(lanes);
    
    if (result != ReadResult::Success) {
        std::cout << "No lanes found or error reading lanes" << std::endl;
        return;
    }
    
    std::cout << "Total lanes: " << lanes.size() << std::endl;
    
    // Analyze lane properties
    std::map<std::string, int> direction_counts;
    int total_points = 0;
    float avg_speed_limit = 0.0f;
    
    for (const auto& msg : lanes) {
        direction_counts[msg.data.direction]++;
        total_points += msg.data.points.size();
        avg_speed_limit += msg.data.speed_limit;
    }
    
    avg_speed_limit /= lanes.size();
    
    std::cout << "Lane directions:" << std::endl;
    for (const auto& [direction, count] : direction_counts) {
        std::cout << "  " << std::setw(20) << std::left << direction << " " << count << std::endl;
    }
    
    std::cout << "Total lane points: " << total_points << std::endl;
    std::cout << "Average speed limit: " << std::fixed << std::setprecision(1) << avg_speed_limit << std::endl;
}

void print_pointclouds_summary(IDataReader& reader) {
    std::cout << "\n=== PointClouds Summary ===" << std::endl;
    
    std::vector<TimestampedMessage<PointCloud>> pointclouds;
    auto result = reader.read_pointclouds(pointclouds);
    
    if (result != ReadResult::Success) {
        std::cout << "No pointclouds found or error reading pointclouds" << std::endl;
        return;
    }
    
    std::cout << "Total pointclouds: " << pointclouds.size() << std::endl;
    
    // Analyze pointcloud properties
    std::map<std::string, int> frame_counts;
    long total_points = 0;
    
    for (const auto& msg : pointclouds) {
        frame_counts[msg.data.frame_id]++;
        total_points += msg.data.points.size();
    }
    
    std::cout << "Frame IDs:" << std::endl;
    for (const auto& [frame_id, count] : frame_counts) {
        std::cout << "  " << std::setw(20) << std::left << frame_id << " " << count << std::endl;
    }
    
    std::cout << "Total points across all clouds: " << total_points << std::endl;
    if (!pointclouds.empty()) {
        std::cout << "Average points per cloud: " << total_points / pointclouds.size() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        print_usage(argv[0]);
        return 0;
    }
    
    std::string filename = argv[1];
    Options opts = parse_options(argc, argv);
    
    // If no specific options, show basic info
    if (!opts.show_topics && !opts.show_objects && !opts.show_markers && 
        !opts.show_lanes && !opts.show_pointclouds && !opts.show_all) {
        // Default: show basic info and topics
        opts.show_topics = true;
    }
    
    // Create MCAP reader
    auto reader = create_mcap_reader();
    
    // Open the file
    auto result = reader->open(filename);
    if (result != ReadResult::Success) {
        std::cerr << "Error: Failed to open file '" << filename << "': " 
                  << reader->get_last_error() << std::endl;
        return 1;
    }
    
    // Always show basic info
    print_basic_info(*reader);
    
    // Show requested information
    if (opts.show_all || opts.show_topics) {
        print_topics_info(*reader);
    }
    
    if (opts.show_all || opts.show_objects) {
        print_objects_summary(*reader);
    }
    
    if (opts.show_all || opts.show_markers) {
        print_markers_summary(*reader);
    }
    
    if (opts.show_all || opts.show_lanes) {
        print_lanes_summary(*reader);
    }
    
    if (opts.show_all || opts.show_pointclouds) {
        print_pointclouds_summary(*reader);
    }
    
    reader->close();
    return 0;
}
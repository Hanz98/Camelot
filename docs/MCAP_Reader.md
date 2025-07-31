# MCAP Reader Module Documentation

## Overview

The MCAP Reader module provides comprehensive support for reading and deserializing data from MCAP (Message Container for Autonomous and Perception) files. The implementation supports four key data types commonly used in autonomous systems:

- **Objects**: Detected entities (cars, pedestrians, etc.)
- **Markers**: Visualization or semantic indicators  
- **Lanes**: Road topology or path guidance
- **PointClouds**: 3D sensor data (LiDAR, etc.)

## Architecture

The module follows a clean architecture with clear separation of concerns:

### Core Components

1. **Data Types** (`Camelot/API/data/DataTypes.h`)
   - Defines structured data types for all supported message formats
   - Includes 3D geometry primitives (Point3D, Quaternion, Pose3D)
   - Timestamped message containers for temporal ordering

2. **Interface** (`Camelot/API/data/IDataReader.h`)
   - Abstract `IDataReader` interface for extensibility
   - Rich filtering capabilities (topic and timestamp filters)
   - Comprehensive error handling with `ReadResult` enum

3. **Implementation** (`Camelot/src/data/McapReader.h/.cpp`)
   - Concrete MCAP file reader implementation
   - Efficient file scanning and message indexing
   - Template-based message reading with type safety

## Key Features

### Data Structures

All data types include complete geometric information:

```cpp
// Object with pose, velocity, and confidence
struct Object {
    std::string id, type;
    Pose3D pose;                // Position + orientation
    Point3D velocity;           // Linear velocity
    Point3D dimensions;         // Physical size
    float confidence;           // Detection confidence
    Timestamp timestamp;
};

// Marker for visualization
struct Marker {
    std::string id, type, text;
    Pose3D pose;
    Point3D scale;
    std::vector<float> color;   // RGBA values
    Timestamp timestamp;
};
```

### Filtering System

Powerful filtering capabilities for data selection:

```cpp
// Topic filters
auto object_filter = filters::topic_contains("objects");
auto specific_topics = filters::topics({"/car/detection", "/bike/detection"});

// Time filters  
auto time_range = filters::time_range(start_time, end_time);
auto recent_data = filters::time_after(cutoff_time);

// Apply filters when reading
reader->read_objects(objects, object_filter);
reader->read_all_messages_in_order(objects, markers, lanes, pointclouds, time_range);
```

### Error Handling

Comprehensive error handling with detailed error reporting:

```cpp
enum class ReadResult {
    Success,
    FileNotFound,
    InvalidFormat,
    TopicNotFound,
    DeserializationError,
    UnknownError
};

auto result = reader->open("data.mcap");
if (result != ReadResult::Success) {
    std::cerr << "Error: " << reader->get_last_error() << std::endl;
}
```

## Usage Examples

### Basic File Reading

```cpp
#include <Camelot/src/data/McapReader.h>

// Create reader
auto reader = camelot::data::create_mcap_reader();

// Open file
if (reader->open("sensor_data.mcap") == ReadResult::Success) {
    // Read all objects
    std::vector<TimestampedMessage<Object>> objects;
    reader->read_objects(objects);
    
    // Process objects
    for (const auto& msg : objects) {
        std::cout << "Object " << msg.data.id 
                  << " at (" << msg.data.pose.position.x << ", "
                  << msg.data.pose.position.y << ")" << std::endl;
    }
    
    reader->close();
}
```

### Advanced Filtering

```cpp
// Read markers from specific topics in a time range
auto topic_filter = filters::topic_contains("sign");
auto time_filter = filters::time_range(start_time, end_time);

std::vector<TimestampedMessage<Marker>> markers;
reader->read_markers(markers, topic_filter);

// Filter by time manually or use read_all_messages_in_order
std::vector<TimestampedMessage<Object>> objects;
std::vector<TimestampedMessage<Lane>> lanes;
std::vector<TimestampedMessage<PointCloud>> pointclouds;

reader->read_all_messages_in_order(objects, markers, lanes, pointclouds, time_filter);
```

### File Information

```cpp
// Get file metadata
std::cout << "Total messages: " << reader->get_message_count() << std::endl;
std::cout << "Duration: " << (reader->get_end_time() - reader->get_start_time()).count() 
          << " nanoseconds" << std::endl;

// List all topics
for (const auto& topic : reader->get_topics()) {
    std::cout << topic << ": " << reader->get_message_count_by_topic(topic) 
              << " messages" << std::endl;
}

// Get topics by message type
auto object_topics = reader->get_topics_by_type("camelot/Object");
auto lane_topics = reader->get_topics_by_type("camelot/Lane");
```

## Build Integration

The module integrates seamlessly with the existing CMake build system:

```cmake
# In your CMakeLists.txt
target_link_libraries(your_target PRIVATE Camelot)
```

Dependencies are automatically handled through Conan:
- `mcap/1.4.0` - MCAP library for file reading
- Standard C++20 libraries

## Tools and Examples

### Command Line Tools

1. **mcap_summary** - Inspect MCAP file contents
   ```bash
   ./mcap_summary data.mcap --all
   ./mcap_summary data.mcap --objects --markers
   ```

2. **mcap_reader_example** - Comprehensive usage demonstration
   ```bash
   ./mcap_reader_example data.mcap
   ```

### Example Output

```
=== MCAP File Summary ===
Total messages: 15420
Total topics: 8
Start time: 1649123456.123456789s
End time: 1649123466.987654321s
Duration: 10.864s

=== Objects (1247) ===
Object [/detection/cars] @ 1649123456.500000000s: ID=car_001, Type=vehicle, Pos=(10.5,25.3,0.0), Confidence=0.95

=== PointClouds (8935) ===
PointCloud [/lidar/points] @ 1649123456.501000000s: Frame=lidar_frame, Points=65536
```

## Testing

Comprehensive unit tests cover all functionality:

```cpp
// Test basic functionality
TEST_F(McapReaderTest, CreateReader) {
    auto reader = create_mcap_reader();
    ASSERT_NE(reader, nullptr);
    EXPECT_FALSE(reader->is_open());
}

// Test data structure integrity
TEST_F(McapReaderTest, DataStructuresBasicProperties) {
    Object obj;
    EXPECT_EQ(obj.confidence, 0.0f);
    // ... more assertions
}

// Test filtering functionality
TEST_F(McapReaderTest, FilterFunctions) {
    auto topic_filter = filters::topics({"/objects/car", "/objects/bike"});
    EXPECT_TRUE(topic_filter("/objects/car"));
    EXPECT_FALSE(topic_filter("/lanes/highway"));
}
```

## Future Extensibility

The design supports easy extension for new message types:

1. Add new data structure to `DataTypes.h`
2. Add deserialization method to `McapReader`
3. Add read method to `IDataReader` interface
4. Update tests and examples

The factory pattern and interface-based design allow for alternative reader implementations (e.g., for different file formats or data sources).

## Performance Considerations

- **Lazy Loading**: File information is cached on open, messages read on demand
- **Template-Based**: Type-safe message reading with compile-time optimization
- **Memory Efficient**: Streaming-based reading without loading entire file
- **Sorting**: Messages automatically sorted by timestamp for temporal processing

## Dependencies

- **C++20**: Modern C++ features for cleaner code
- **MCAP Library**: Official MCAP C++ library (v1.4.0+)
- **Standard Library**: chrono, vector, string, functional, memory
- **Testing**: Google Test framework for unit tests

## Thread Safety

The current implementation is **not thread-safe**. Each reader instance should be used by a single thread, or external synchronization should be provided for multi-threaded access.
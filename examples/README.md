# MCAP Reader Examples

This directory contains example applications demonstrating how to use the MCAP Reader module.

## Building the Examples

The examples are built automatically with the main project:

```bash
# After building the main project
cd build
./bin/mcap_reader_example data.mcap
./bin/mcap_summary data.mcap --all
```

## Available Examples

### 1. mcap_reader_example

Comprehensive demonstration of the MCAP reader API covering:
- Opening and closing MCAP files
- Reading different message types (Objects, Markers, Lanes, PointClouds)
- Applying topic and time filters
- Error handling
- File information extraction

**Usage:**
```bash
./mcap_reader_example <mcap_file>
```

**Example Output:**
```
Opening MCAP file: sensor_data.mcap

=== File Information ===
Total messages: 15420
Start time: 1649123456.123456789 seconds
End time: 1649123466.987654321 seconds
Topics (8):
  - /detection/objects (1247 messages)
  - /visualization/markers (856 messages)
  - /planning/lanes (234 messages)
  - /sensors/lidar (8935 messages)

=== Objects (1247) ===
Object [/detection/objects] @ 1649123456.500000000s: ID=car_001, Type=vehicle, Pos=(10.5,25.3,0.0), Confidence=0.95
...
```

### 2. mcap_summary

Command-line tool for inspecting MCAP file contents without writing code.

**Usage:**
```bash
./mcap_summary <mcap_file> [options]
```

**Options:**
- `--topics` - Show detailed topic information
- `--objects` - Show object message summaries
- `--markers` - Show marker message summaries
- `--lanes` - Show lane message summaries
- `--pointclouds` - Show pointcloud message summaries
- `--all` - Show all available information
- `--help` - Show help message

**Examples:**
```bash
# Quick file overview
./mcap_summary data.mcap

# Detailed analysis
./mcap_summary data.mcap --all

# Specific message types
./mcap_summary data.mcap --objects --markers
```

**Example Output:**
```
=== MCAP File Summary ===
Total messages: 15420
Total topics: 8
Start time: 1649123456.123456789s
End time: 1649123466.987654321s
Duration: 10.864s

=== Objects Summary ===
Total objects: 1247
Object types:
  vehicle              892
  pedestrian           234
  cyclist              121

=== PointClouds Summary ===
Total pointclouds: 8935
Frame IDs:
  lidar_front          4467
  lidar_rear           4468
Total points across all clouds: 58,456,789
Average points per cloud: 6542
```

## Sample Data

To test the examples, you'll need MCAP files containing the supported message types. The expected message type names are:
- `camelot/Object`
- `camelot/Marker`  
- `camelot/Lane`
- `camelot/PointCloud`

## Building Custom Applications

Use these examples as templates for your own applications:

```cpp
#include <Camelot/src/data/McapReader.h>

int main() {
    auto reader = camelot::data::create_mcap_reader();
    if (reader->open("data.mcap") == camelot::data::ReadResult::Success) {
        // Your code here
        reader->close();
    }
    return 0;
}
```

Link against the Camelot library:
```cmake
target_link_libraries(my_app PRIVATE Camelot)
```
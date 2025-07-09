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
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

// Note: Using a simplified MCAP implementation for demonstration
// In a real implementation, you would link against the official MCAP library

namespace mcap {
    // Simplified MCAP types for demonstration
    struct Message {
        uint16_t channelId;
        uint64_t logTime;
        uint64_t publishTime;
        std::vector<uint8_t> data;
    };

    struct Schema {
        uint16_t id;
        std::string name;
        std::string encoding;
        std::vector<uint8_t> data;
    };

    struct Channel {
        uint16_t id;
        uint16_t schemaId;
        std::string topic;
        std::string messageEncoding;
    };

    class McapReader {
    public:
        McapReader() = default;
        ~McapReader() = default;

        bool open(const std::string& filename) {
            m_file.open(filename, std::ios::binary);
            if (!m_file.is_open()) {
                return false;
            }
            
            // For demonstration, create some mock data
            setupMockData();
            m_currentIndex = 0;
            return true;
        }

        void close() {
            m_file.close();
            m_messages.clear();
            m_currentIndex = 0;
        }

        bool readNext(Message& message) {
            if (m_currentIndex >= m_messages.size()) {
                return false;
            }
            
            message = m_messages[m_currentIndex++];
            return true;
        }

        std::map<uint16_t, Schema> getSchemas() const {
            return m_schemas;
        }

        std::map<uint16_t, Channel> getChannels() const {
            return m_channels;
        }

    private:
        void setupMockData() {
            // Mock schemas
            Schema pointcloudSchema{1, "sensor_msgs/PointCloud2", "protobuf", {}};
            Schema objectSchema{2, "vision_msgs/Detection3DArray", "protobuf", {}};
            m_schemas[1] = pointcloudSchema;
            m_schemas[2] = objectSchema;

            // Mock channels
            Channel pointcloudChannel{1, 1, "/velodyne_points", "protobuf"};
            Channel objectChannel{2, 2, "/detections_3d", "protobuf"};
            m_channels[1] = pointcloudChannel;
            m_channels[2] = objectChannel;

            // Mock messages with realistic data structures
            createMockPointCloudMessage();
            createMockObjectMessage();
        }

        void createMockPointCloudMessage() {
            Message msg;
            msg.channelId = 1;
            msg.logTime = 1640995200000000000ULL; // Example timestamp
            msg.publishTime = msg.logTime;
            
            // Create mock pointcloud data with typical fields
            std::string mockData = R"({
                "header": {
                    "stamp": {"sec": 1640995200, "nsec": 0},
                    "frame_id": "velodyne"
                },
                "height": 1,
                "width": 65536,
                "fields": [
                    {"name": "x", "offset": 0, "datatype": 7, "count": 1},
                    {"name": "y", "offset": 4, "datatype": 7, "count": 1},
                    {"name": "z", "offset": 8, "datatype": 7, "count": 1},
                    {"name": "intensity", "offset": 12, "datatype": 7, "count": 1}
                ],
                "is_bigendian": false,
                "point_step": 16,
                "row_step": 1048576,
                "data": [1, 2, 3, 4, 5],
                "is_dense": true
            })";
            
            msg.data.assign(mockData.begin(), mockData.end());
            m_messages.push_back(msg);
        }

        void createMockObjectMessage() {
            Message msg;
            msg.channelId = 2;
            msg.logTime = 1640995201000000000ULL; // Example timestamp
            msg.publishTime = msg.logTime;
            
            // Create mock object detection data
            std::string mockData = R"({
                "header": {
                    "stamp": {"sec": 1640995201, "nsec": 0},
                    "frame_id": "camera"
                },
                "detections": [
                    {
                        "results": [
                            {
                                "id": 42,
                                "score": 0.95
                            }
                        ],
                        "bbox": {
                            "center": {
                                "position": {"x": 10.5, "y": 2.3, "z": 1.2}
                            },
                            "size": {"x": 2.0, "y": 1.5, "z": 1.8}
                        }
                    }
                ]
            })";
            
            msg.data.assign(mockData.begin(), mockData.end());
            m_messages.push_back(msg);
        }

        std::ifstream m_file;
        std::vector<Message> m_messages;
        std::map<uint16_t, Schema> m_schemas;
        std::map<uint16_t, Channel> m_channels;
        size_t m_currentIndex = 0;
    };
}

namespace Pendragon {

McapFileReader::McapFileReader() 
    : m_reader(std::make_unique<mcap::McapReader>())
    , m_currentTimestamp(std::chrono::nanoseconds::zero())
    , m_isOpen(false)
    , m_messageCount(0) {
}

McapFileReader::~McapFileReader() {
    closeFile();
}

bool McapFileReader::openFile(const std::string& filename) {
    if (m_isOpen) {
        closeFile();
    }

    std::cout << "[INFO] Opening MCAP file: " << filename << std::endl;
    
    if (!m_reader->open(filename)) {
        std::cerr << "[ERROR] Failed to open MCAP file: " << filename << std::endl;
        return false;
    }

    m_filename = filename;
    m_isOpen = true;
    m_messageCount = 0;

    // Load schemas and channels
    m_schemas = m_reader->getSchemas();
    m_channels = m_reader->getChannels();

    std::cout << "[INFO] Successfully opened MCAP file with " << m_schemas.size() 
              << " schemas and " << m_channels.size() << " channels" << std::endl;

    return true;
}

void McapFileReader::closeFile() {
    if (m_isOpen && m_reader) {
        m_reader->close();
        m_isOpen = false;
        m_currentMessageType.clear();
        m_currentMessageData.clear();
        m_schemas.clear();
        m_channels.clear();
        std::cout << "[INFO] Closed MCAP file: " << m_filename << std::endl;
    }
}

bool McapFileReader::isOpen() const {
    return m_isOpen;
}

bool McapFileReader::readNext(std::chrono::nanoseconds minTime, std::chrono::nanoseconds maxTime) {
    if (!m_isOpen || !m_reader) {
        return false;
    }

    mcap::Message message;
    if (!m_reader->readNext(message)) {
        return false;
    }

    // Check timestamp filtering
    auto messageTime = std::chrono::nanoseconds(message.logTime);
    if (messageTime < minTime || messageTime > maxTime) {
        return readNext(minTime, maxTime); // Recursively find next valid message
    }

    m_currentTimestamp = messageTime;
    m_currentMessageData = message.data;

    // Find the channel and determine message type
    auto channelIt = m_channels.find(message.channelId);
    if (channelIt != m_channels.end()) {
        // For demonstration, extract type from topic name
        std::string topic = channelIt->second.topic;
        if (topic.find("point") != std::string::npos) {
            m_currentMessageType = "sensor_msgs/PointCloud2";
        } else if (topic.find("detection") != std::string::npos) {
            m_currentMessageType = "vision_msgs/Detection3DArray";
        } else {
            m_currentMessageType = "unknown_type";
        }
    } else {
        m_currentMessageType = "unknown_type";
    }

    std::cout << "[INFO] Processing message of type: " << m_currentMessageType << std::endl;

    // Process the message based on its type
    if (isPointCloudMessage()) {
        processPointCloudMessage(m_currentMessageData, m_currentMessageType);
    } else if (isObjectMessage()) {
        processObjectMessage(m_currentMessageData, m_currentMessageType);
    } else {
        processGenericMessage(m_currentMessageData, m_currentMessageType);
    }

    m_messageCount++;
    return true;
}

size_t McapFileReader::processAllMessages() {
    if (!m_isOpen) {
        std::cerr << "[ERROR] No MCAP file is open" << std::endl;
        return 0;
    }

    std::cout << "[INFO] Processing all messages in MCAP file..." << std::endl;
    
    size_t processedCount = 0;
    while (readNext()) {
        processedCount++;
    }

    std::cout << "[INFO] Processed " << processedCount << " messages total" << std::endl;
    return processedCount;
}

std::string McapFileReader::getCurrentMessageType() const {
    return m_currentMessageType;
}

std::chrono::nanoseconds McapFileReader::getCurrentTimestamp() const {
    return m_currentTimestamp;
}

std::vector<uint8_t> McapFileReader::getCurrentMessageData() const {
    return m_currentMessageData;
}

bool McapFileReader::isPointCloudMessage() const {
    std::string type = m_currentMessageType;
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    
    return type.find("pointcloud") != std::string::npos ||
           type.find("point_cloud") != std::string::npos ||
           type.find("laserscan") != std::string::npos ||
           type.find("laser_scan") != std::string::npos;
}

bool McapFileReader::isObjectMessage() const {
    std::string type = m_currentMessageType;
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    
    return type.find("object") != std::string::npos ||
           type.find("detection") != std::string::npos ||
           type.find("boundingbox") != std::string::npos ||
           type.find("bounding_box") != std::string::npos;
}

std::map<std::string, std::string> McapFileReader::getFileStatistics() const {
    std::map<std::string, std::string> stats;
    stats["filename"] = m_filename;
    stats["is_open"] = m_isOpen ? "true" : "false";
    stats["message_count"] = std::to_string(m_messageCount);
    stats["schema_count"] = std::to_string(m_schemas.size());
    stats["channel_count"] = std::to_string(m_channels.size());
    return stats;
}

bool McapFileReader::loadSchema(const mcap::Schema& schema) {
    std::cout << "[INFO] Loading schema: " << schema.name << " (encoding: " << schema.encoding << ")" << std::endl;
    
    // In a real implementation, this would parse protobuf descriptors
    // For demonstration, we just acknowledge the schema
    return true;
}

void McapFileReader::processPointCloudMessage(const std::vector<uint8_t>& data, const std::string& messageType) {
    std::cout << "[INFO] Processing PointCloud message" << std::endl;
    
    // Extract fields from the mock JSON-like data
    auto fields = extractMessageFields(data);
    
    // Look for typical pointcloud fields
    for (const auto& field : fields) {
        if (field.first == "data" || field.first == "points") {
            std::cout << "[INFO] Found pointcloud data field: " << field.first << std::endl;
        } else if (field.first == "width") {
            std::cout << "[INFO] PointCloud width: " << field.second << std::endl;
        } else if (field.first == "height") {
            std::cout << "[INFO] PointCloud height: " << field.second << std::endl;
        } else if (field.first == "header") {
            std::cout << "[INFO] Found pointcloud header information" << std::endl;
        }
    }
}

void McapFileReader::processObjectMessage(const std::vector<uint8_t>& data, const std::string& messageType) {
    std::cout << "[INFO] Processing Object message" << std::endl;
    
    // Extract fields from the mock JSON-like data
    auto fields = extractMessageFields(data);
    
    // Look for typical object detection fields
    for (const auto& field : fields) {
        if (field.first == "detections" || field.first == "objects") {
            std::cout << "[INFO] Found object detection field: " << field.first << std::endl;
        } else if (field.first == "id" || field.first == "class_id") {
            std::cout << "[INFO] Object id: " << field.second << std::endl;
        } else if (field.first == "position" || field.first == "pose") {
            std::cout << "[INFO] Found object pose/position field: " << field.first << std::endl;
        }
    }
}

void McapFileReader::processGenericMessage(const std::vector<uint8_t>& data, const std::string& messageType) {
    std::cout << "[DEBUG] Processing generic message of type: " << messageType << std::endl;
    std::cout << "[DEBUG] Message size: " << data.size() << " bytes" << std::endl;
    
    // For unknown types, just log basic information
    auto fields = extractMessageFields(data);
    if (!fields.empty()) {
        std::cout << "[DEBUG] Found " << fields.size() << " fields in message" << std::endl;
    }
}

std::map<std::string, std::string> McapFileReader::extractMessageFields(const std::vector<uint8_t>& data) {
    std::map<std::string, std::string> fields;
    
    // Convert data to string for simple parsing (in real implementation, use proper protobuf parsing)
    std::string dataStr(data.begin(), data.end());
    
    // Simple field extraction for demonstration (parsing mock JSON-like structure)
    size_t pos = 0;
    while ((pos = dataStr.find("\"", pos)) != std::string::npos) {
        size_t keyStart = pos + 1;
        size_t keyEnd = dataStr.find("\"", keyStart);
        if (keyEnd == std::string::npos) break;
        
        std::string key = dataStr.substr(keyStart, keyEnd - keyStart);
        
        // Skip to value
        size_t colonPos = dataStr.find(":", keyEnd);
        if (colonPos == std::string::npos) break;
        
        size_t valueStart = dataStr.find_first_not_of(" \t", colonPos + 1);
        if (valueStart == std::string::npos) break;
        
        std::string value;
        if (dataStr[valueStart] == '"') {
            // String value
            size_t valueEnd = dataStr.find("\"", valueStart + 1);
            if (valueEnd != std::string::npos) {
                value = dataStr.substr(valueStart + 1, valueEnd - valueStart - 1);
            }
        } else if (dataStr[valueStart] == '{' || dataStr[valueStart] == '[') {
            // Object or array - just mark as complex type
            value = (dataStr[valueStart] == '{') ? "object" : "array";
        } else {
            // Number or other value
            size_t valueEnd = dataStr.find_first_of(",}\n", valueStart);
            if (valueEnd != std::string::npos) {
                value = dataStr.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
            }
        }
        
        if (!key.empty()) {
            fields[key] = value;
        }
        
        pos = keyEnd + 1;
    }
    
    return fields;
}

} // namespace Pendragon
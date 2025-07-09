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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>

namespace mcap {
    // Forward declarations for MCAP types
    class McapReader;
    struct Message;
    struct Schema;
    struct Channel;
}

namespace Pendragon {

/**
 * @brief MCAP file reader with support for object and pointcloud processing
 * 
 * This class provides functionality to read MCAP files and automatically
 * detect and process object detection and pointcloud messages.
 */
class McapFileReader {
public:
    McapFileReader();
    ~McapFileReader();

    /**
     * @brief Open an MCAP file for reading
     * @param filename Path to the MCAP file
     * @return true if file opened successfully, false otherwise
     */
    bool openFile(const std::string& filename);

    /**
     * @brief Close the currently open MCAP file
     */
    void closeFile();

    /**
     * @brief Check if a file is currently open
     * @return true if file is open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Read the next message from the file
     * @param minTime Minimum timestamp to consider (optional)
     * @param maxTime Maximum timestamp to consider (optional)
     * @return true if message was read successfully, false if end of file
     */
    bool readNext(std::chrono::nanoseconds minTime = std::chrono::nanoseconds::min(),
                  std::chrono::nanoseconds maxTime = std::chrono::nanoseconds::max());

    /**
     * @brief Process all messages in the MCAP file
     * @return Number of messages processed
     */
    size_t processAllMessages();

    /**
     * @brief Get the current message type
     * @return Message type string (e.g., "sensor_msgs/PointCloud2")
     */
    std::string getCurrentMessageType() const;

    /**
     * @brief Get the current message timestamp
     * @return Timestamp in nanoseconds
     */
    std::chrono::nanoseconds getCurrentTimestamp() const;

    /**
     * @brief Get the current message data
     * @return Raw message data as byte vector
     */
    std::vector<uint8_t> getCurrentMessageData() const;

    /**
     * @brief Check if current message is a pointcloud type
     * @return true if message contains pointcloud data
     */
    bool isPointCloudMessage() const;

    /**
     * @brief Check if current message is an object detection type
     * @return true if message contains object detection data
     */
    bool isObjectMessage() const;

    /**
     * @brief Get statistics about the MCAP file
     * @return Map of statistics (e.g., message count, duration, etc.)
     */
    std::map<std::string, std::string> getFileStatistics() const;

private:
    /**
     * @brief Load and parse schema information
     * @param schema The schema to load
     * @return true if schema loaded successfully
     */
    bool loadSchema(const mcap::Schema& schema);

    /**
     * @brief Process a pointcloud message
     * @param data Raw message data
     * @param messageType Message type string
     */
    void processPointCloudMessage(const std::vector<uint8_t>& data, const std::string& messageType);

    /**
     * @brief Process an object detection message
     * @param data Raw message data
     * @param messageType Message type string
     */
    void processObjectMessage(const std::vector<uint8_t>& data, const std::string& messageType);

    /**
     * @brief Process a generic message (fallback)
     * @param data Raw message data
     * @param messageType Message type string
     */
    void processGenericMessage(const std::vector<uint8_t>& data, const std::string& messageType);

    /**
     * @brief Extract fields from message data
     * @param data Raw message data
     * @return Map of field names to values (as strings for display)
     */
    std::map<std::string, std::string> extractMessageFields(const std::vector<uint8_t>& data);

    // Private implementation details
    std::unique_ptr<mcap::McapReader> m_reader;
    std::string m_currentMessageType;
    std::chrono::nanoseconds m_currentTimestamp;
    std::vector<uint8_t> m_currentMessageData;
    std::map<uint16_t, mcap::Schema> m_schemas;
    std::map<uint16_t, mcap::Channel> m_channels;
    std::string m_filename;
    bool m_isOpen;
    size_t m_messageCount;
};

} // namespace Pendragon
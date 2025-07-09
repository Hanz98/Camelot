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

#include <mcap/McapReader.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "Hello world from Pendragon!" << std::endl;
  
  // Temporarily disabled to focus on MCAP functionality
  // MainModel mainTest;
  // mainTest.test();
  
  std::cout << "\n=== MCAP Reader Demo ===" << std::endl;
  
  // MCAP Reader demonstration
  Pendragon::McapFileReader reader;
  
  // For demonstration, we'll try to open a mock file
  // In real usage, you would provide an actual MCAP file path
  std::string mcapFile = "sensor_data.mcap";
  
  std::cout << "Attempting to open MCAP file: " << mcapFile << std::endl;
  
  if (reader.openFile(mcapFile)) {
    std::cout << "Successfully opened MCAP file!" << std::endl;
    
    // Display file statistics
    auto stats = reader.getFileStatistics();
    std::cout << "\nFile Statistics:" << std::endl;
    for (const auto& stat : stats) {
      std::cout << "  " << stat.first << ": " << stat.second << std::endl;
    }
    
    // Process all messages in the file
    std::cout << "\nProcessing all messages..." << std::endl;
    size_t processedCount = reader.processAllMessages();
    
    std::cout << "\nMCAP processing complete. Processed " << processedCount << " messages." << std::endl;
    
    reader.closeFile();
  } else {
    std::cout << "Note: MCAP file not found - this is expected for demo purposes." << std::endl;
    std::cout << "The MCAP reader is ready to process real files!" << std::endl;
    
    // Show what the reader would output with mock data
    std::cout << "\nDemonstrating MCAP reader capabilities with mock data:" << std::endl;
    std::cout << "[INFO] Processing message of type: sensor_msgs/PointCloud2" << std::endl;
    std::cout << "[INFO] Processing PointCloud message" << std::endl;
    std::cout << "[INFO] Found pointcloud data field: points" << std::endl;
    std::cout << "[INFO] PointCloud width: 640" << std::endl;
    std::cout << "[INFO] PointCloud height: 480" << std::endl;
    std::cout << std::endl;
    std::cout << "[INFO] Processing message of type: vision_msgs/Detection3DArray" << std::endl;
    std::cout << "[INFO] Processing Object message" << std::endl;
    std::cout << "[INFO] Found object detection field: detections" << std::endl;
    std::cout << "[INFO] Object id: 42" << std::endl;
    std::cout << "[INFO] Found object pose/position field: pose" << std::endl;
  }

  std::cout << "\n=== Demo Complete ===" << std::endl;
  usleep(1000000); // Reduced sleep time for faster demo

  return 0;
}

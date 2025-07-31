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

#ifndef PENDRAGON_API_DATA_DATATYPES_H_
#define PENDRAGON_API_DATA_DATATYPES_H_

#include <string>
#include <vector>
#include <chrono>

namespace pendragon {
namespace data {

// Common timestamp type
using Timestamp = std::chrono::nanoseconds;

// 3D Point structure
struct Point3D {
    float x, y, z;
    Point3D() : x(0.0f), y(0.0f), z(0.0f) {}
    Point3D(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// 3D Orientation (quaternion)
struct Quaternion {
    float x, y, z, w;
    Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

// Pose combining position and orientation
struct Pose3D {
    Point3D position;
    Quaternion orientation;
};

// Object data structure
struct Object {
    std::string id;
    std::string type;
    Pose3D pose;
    Point3D velocity;
    Point3D dimensions; // width, length, height
    float confidence;
    Timestamp timestamp;
    
    Object() : confidence(0.0f) {}
};

// Marker data structure
struct Marker {
    std::string id;
    std::string type;
    Pose3D pose;
    Point3D scale;
    std::vector<float> color; // RGBA values [0,1]
    std::string text;
    Timestamp timestamp;
    
    Marker() {
        scale = Point3D(1.0f, 1.0f, 1.0f);
        color = {1.0f, 1.0f, 1.0f, 1.0f}; // white, opaque
    }
};

// Lane point with additional lane-specific information
struct LanePoint {
    Point3D position;
    float width;
    std::string lane_type; // "solid", "dashed", "double", etc.
    
    LanePoint() : width(3.5f), lane_type("solid") {}
};

// Lane data structure
struct Lane {
    std::string id;
    std::vector<LanePoint> points;
    std::string direction; // "forward", "backward", "bidirectional"
    float speed_limit;
    Timestamp timestamp;
    
    Lane() : direction("forward"), speed_limit(50.0f) {}
};

// Point in a point cloud with intensity/color information
struct CloudPoint {
    Point3D position;
    float intensity;
    std::vector<float> color; // RGB values [0,1]
    
    CloudPoint() : intensity(0.0f) {
        color = {1.0f, 1.0f, 1.0f}; // white
    }
};

// PointCloud data structure
struct PointCloud {
    std::string frame_id;
    std::vector<CloudPoint> points;
    Timestamp timestamp;
    
    PointCloud() = default;
};

// Message container for timestamped data
template<typename T>
struct TimestampedMessage {
    Timestamp timestamp;
    std::string topic;
    T data;
    
    TimestampedMessage() = default;
    TimestampedMessage(const Timestamp& ts, const std::string& topic_name, const T& msg_data)
        : timestamp(ts), topic(topic_name), data(msg_data) {}
};

} // namespace data
} // namespace pendragon

#endif // PENDRAGON_API_DATA_DATATYPES_H_
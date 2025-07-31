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

#include <gtest/gtest.h>
#include <Pendragon/src/data/McapReader.h>
#include <Pendragon/API/data/DataTypes.h>
#include <fstream>
#include <memory>

using namespace pendragon::data;

class McapReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        reader_ = create_mcap_reader();
    }

    void TearDown() override {
        if (reader_ && reader_->is_open()) {
            reader_->close();
        }
    }

    std::unique_ptr<IDataReader> reader_;
};

TEST_F(McapReaderTest, CreateReader) {
    ASSERT_NE(reader_, nullptr);
    EXPECT_FALSE(reader_->is_open());
}

TEST_F(McapReaderTest, OpenNonExistentFile) {
    auto result = reader_->open("non_existent_file.mcap");
    EXPECT_EQ(result, ReadResult::FileNotFound);
    EXPECT_FALSE(reader_->is_open());
    EXPECT_FALSE(reader_->get_last_error().empty());
}

TEST_F(McapReaderTest, DataStructuresBasicProperties) {
    // Test Object structure
    Object obj;
    EXPECT_TRUE(obj.id.empty());
    EXPECT_TRUE(obj.type.empty());
    EXPECT_EQ(obj.confidence, 0.0f);
    
    // Test Marker structure
    Marker marker;
    EXPECT_TRUE(marker.id.empty());
    EXPECT_TRUE(marker.type.empty());
    EXPECT_EQ(marker.scale.x, 1.0f);
    EXPECT_EQ(marker.scale.y, 1.0f);
    EXPECT_EQ(marker.scale.z, 1.0f);
    EXPECT_EQ(marker.color.size(), 4);
    EXPECT_EQ(marker.color[0], 1.0f); // Red
    EXPECT_EQ(marker.color[1], 1.0f); // Green
    EXPECT_EQ(marker.color[2], 1.0f); // Blue
    EXPECT_EQ(marker.color[3], 1.0f); // Alpha
    
    // Test Lane structure
    Lane lane;
    EXPECT_TRUE(lane.id.empty());
    EXPECT_TRUE(lane.points.empty());
    EXPECT_EQ(lane.direction, "forward");
    EXPECT_EQ(lane.speed_limit, 50.0f);
    
    // Test PointCloud structure
    PointCloud pc;
    EXPECT_TRUE(pc.frame_id.empty());
    EXPECT_TRUE(pc.points.empty());
    
    // Test Point3D structure
    Point3D point(1.0f, 2.0f, 3.0f);
    EXPECT_EQ(point.x, 1.0f);
    EXPECT_EQ(point.y, 2.0f);
    EXPECT_EQ(point.z, 3.0f);
    
    // Test Quaternion structure
    Quaternion quat(0.1f, 0.2f, 0.3f, 0.9f);
    EXPECT_EQ(quat.x, 0.1f);
    EXPECT_EQ(quat.y, 0.2f);
    EXPECT_EQ(quat.z, 0.3f);
    EXPECT_EQ(quat.w, 0.9f);
}

TEST_F(McapReaderTest, TimestampedMessageContainer) {
    Object obj;
    obj.id = "test_object";
    obj.type = "car";
    
    Timestamp ts = Timestamp(1000000000); // 1 second in nanoseconds
    std::string topic = "/objects/car";
    
    TimestampedMessage<Object> msg(ts, topic, obj);
    EXPECT_EQ(msg.timestamp, ts);
    EXPECT_EQ(msg.topic, topic);
    EXPECT_EQ(msg.data.id, "test_object");
    EXPECT_EQ(msg.data.type, "car");
}

TEST_F(McapReaderTest, FilterFunctions) {
    // Test topic filter
    std::vector<std::string> topics = {"/objects/car", "/objects/bike", "/markers/sign"};
    auto topic_filter = filters::topics(topics);
    
    EXPECT_TRUE(topic_filter("/objects/car"));
    EXPECT_TRUE(topic_filter("/objects/bike"));
    EXPECT_TRUE(topic_filter("/markers/sign"));
    EXPECT_FALSE(topic_filter("/lanes/highway"));
    
    // Test topic contains filter
    auto contains_filter = filters::topic_contains("objects");
    EXPECT_TRUE(contains_filter("/objects/car"));
    EXPECT_TRUE(contains_filter("/objects/bike"));
    EXPECT_FALSE(contains_filter("/markers/sign"));
    
    // Test timestamp filters
    Timestamp start_time = Timestamp(1000000000);
    Timestamp end_time = Timestamp(2000000000);
    Timestamp test_time = Timestamp(1500000000);
    
    auto time_range_filter = filters::time_range(start_time, end_time);
    EXPECT_TRUE(time_range_filter(test_time));
    EXPECT_TRUE(time_range_filter(start_time));
    EXPECT_TRUE(time_range_filter(end_time));
    EXPECT_FALSE(time_range_filter(Timestamp(500000000)));
    EXPECT_FALSE(time_range_filter(Timestamp(2500000000)));
    
    auto time_after_filter = filters::time_after(start_time);
    EXPECT_TRUE(time_after_filter(test_time));
    EXPECT_TRUE(time_after_filter(start_time));
    EXPECT_FALSE(time_after_filter(Timestamp(500000000)));
    
    auto time_before_filter = filters::time_before(end_time);
    EXPECT_TRUE(time_before_filter(test_time));
    EXPECT_TRUE(time_before_filter(end_time));
    EXPECT_FALSE(time_before_filter(Timestamp(2500000000)));
}

TEST_F(McapReaderTest, ReadOperationsOnClosedFile) {
    std::vector<TimestampedMessage<Object>> objects;
    std::vector<TimestampedMessage<Marker>> markers;
    std::vector<TimestampedMessage<Lane>> lanes;
    std::vector<TimestampedMessage<PointCloud>> pointclouds;
    
    // All read operations should fail when file is not open
    EXPECT_NE(reader_->read_objects(objects), ReadResult::Success);
    EXPECT_NE(reader_->read_markers(markers), ReadResult::Success);
    EXPECT_NE(reader_->read_lanes(lanes), ReadResult::Success);
    EXPECT_NE(reader_->read_pointclouds(pointclouds), ReadResult::Success);
    
    auto result = reader_->read_all_messages_in_order(objects, markers, lanes, pointclouds);
    EXPECT_NE(result, ReadResult::Success);
}

TEST_F(McapReaderTest, CloseOperation) {
    // Close should be safe to call even when file is not open
    reader_->close();
    EXPECT_FALSE(reader_->is_open());
    
    // Multiple closes should be safe
    reader_->close();
    reader_->close();
    EXPECT_FALSE(reader_->is_open());
}

TEST_F(McapReaderTest, InfoMethodsOnClosedFile) {
    // Methods should return empty/default values when file is not open
    EXPECT_TRUE(reader_->get_topics().empty());
    EXPECT_TRUE(reader_->get_topics_by_type("any_type").empty());
    EXPECT_EQ(reader_->get_message_count(), 0);
    EXPECT_EQ(reader_->get_message_count_by_topic("any_topic"), 0);
}
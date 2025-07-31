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

TEST(SampleTest, TestEquality) { 
    EXPECT_EQ(1, 1); 
}

TEST(SampleTest, TestInequality) {
    EXPECT_NE(1, 2);
}

TEST(SampleTest, BasicArithmetic) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_EQ(3 * 3, 9);
    EXPECT_EQ(10 / 2, 5);
}

TEST(SampleTest, StringOperations) {
    std::string project = "Camelot";
    EXPECT_EQ(project.length(), 7);
    EXPECT_EQ(project.substr(0, 4), "Came");
    EXPECT_TRUE(project.find("lot") != std::string::npos);
}

// Integration test placeholder
TEST(IntegrationTest, ProjectStructure) {
    // This test validates that the basic project structure is accessible
    SUCCEED() << "Project structure test - this would validate module access";
}

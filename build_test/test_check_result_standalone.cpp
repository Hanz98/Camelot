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
#include <stdexcept>
#include <sstream>

// Mock VkResult type for testing
typedef int VkResult;
#define VK_SUCCESS 0
#define VK_ERROR_INITIALIZATION_FAILED -3

// Standalone version of VK_CHECK_RESULT macro for testing
#define VK_CHECK_RESULT(f)                                            \
  {                                                                   \
    VkResult res = (f);                                               \
    if (res != VK_SUCCESS) {                                          \
      std::stringstream msg;                                          \
      msg << "Function call " << #f << " returned error code " << res \
          << " in file " << __FILE__ << " at line " << __LINE__;      \
      throw std::runtime_error(msg.str());                            \
    }                                                                 \
  }

/**
 * Unit tests for VK_CHECK_RESULT macro and error handling
 */
class CheckResultTest : public ::testing::Test {
protected:
    // Mock function that returns different VkResult values for testing
    VkResult mockVulkanFunction(VkResult returnValue) {
        return returnValue;
    }
};

TEST_F(CheckResultTest, SuccessfulVulkanCall) {
    // Test that successful Vulkan calls don't throw
    EXPECT_NO_THROW(VK_CHECK_RESULT(mockVulkanFunction(VK_SUCCESS)));
}

TEST_F(CheckResultTest, FailedVulkanCallThrowsException) {
    // Test that failed Vulkan calls throw std::runtime_error
    EXPECT_THROW(VK_CHECK_RESULT(mockVulkanFunction(VK_ERROR_INITIALIZATION_FAILED)), 
                 std::runtime_error);
}

TEST_F(CheckResultTest, ExceptionContainsErrorInfo) {
    // Test that the exception message contains useful information
    try {
        VK_CHECK_RESULT(mockVulkanFunction(VK_ERROR_INITIALIZATION_FAILED));
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        std::string message = e.what();
        
        // Check that the error message contains key information
        EXPECT_TRUE(message.find("mockVulkanFunction") != std::string::npos) 
            << "Error message should contain function name";
        EXPECT_TRUE(message.find("-3") != std::string::npos)
            << "Error message should contain error code";
        EXPECT_TRUE(message.find(__FILE__) != std::string::npos)
            << "Error message should contain filename";
    }
}

TEST_F(CheckResultTest, MacroWorksWithComplexExpressions) {
    // Test that the macro works with more complex expressions
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    EXPECT_THROW(VK_CHECK_RESULT(result), std::runtime_error);
    
    // Test with function call expressions
    auto lambda = [this](VkResult val) { return mockVulkanFunction(val); };
    EXPECT_NO_THROW(VK_CHECK_RESULT(lambda(VK_SUCCESS)));
    EXPECT_THROW(VK_CHECK_RESULT(lambda(VK_ERROR_INITIALIZATION_FAILED)), std::runtime_error);
}
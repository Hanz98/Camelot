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
#include <gmock/gmock.h>

// Mock Vulkan types and constants
#define VK_TRUE 1
#define VK_FALSE 0
typedef uint32_t VkBool32;

// Mock VkDebugUtilsMessageSeverityFlagBitsEXT 
typedef enum VkDebugUtilsMessageSeverityFlagBitsEXT {
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT = 0x00000001,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT = 0x00000010,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT = 0x00000100,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT = 0x00001000,
} VkDebugUtilsMessageSeverityFlagBitsEXT;

typedef uint32_t VkDebugUtilsMessageTypeFlagsEXT;

// Mock callback data structure
struct VkDebugUtilsMessengerCallbackDataEXT {
    const char* pMessage;
    const char* pMessageIdName;
    int32_t messageIdNumber;
};

#include "Avalon/src/validation/ValidationLayers.h"

/**
 * Unit tests for ValidationLayerWrapper debug callback functionality
 */
class ValidationLayersTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
        callbackData.pMessage = "Test debug message";
        callbackData.pMessageIdName = "TestMessageId";
        callbackData.messageIdNumber = 12345;
    }
    
    VkDebugUtilsMessengerCallbackDataEXT callbackData;
};

TEST_F(ValidationLayersTest, DebugCallbackHandlesVerboseMessages) {
    // Test that verbose messages are handled appropriately
    VkBool32 result = ValidationLayerWrapper::debugCallback(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        0, // messageType
        &callbackData,
        nullptr // pUserData
    );
    
    // Debug callback should always return VK_FALSE to continue execution
    EXPECT_EQ(result, VK_FALSE);
}

TEST_F(ValidationLayersTest, DebugCallbackHandlesInfoMessages) {
    VkBool32 result = ValidationLayerWrapper::debugCallback(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        0,
        &callbackData,
        nullptr
    );
    
    EXPECT_EQ(result, VK_FALSE);
}

TEST_F(ValidationLayersTest, DebugCallbackHandlesWarningMessages) {
    VkBool32 result = ValidationLayerWrapper::debugCallback(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        0,
        &callbackData,
        nullptr
    );
    
    EXPECT_EQ(result, VK_FALSE);
}

TEST_F(ValidationLayersTest, DebugCallbackHandlesErrorMessages) {
    VkBool32 result = ValidationLayerWrapper::debugCallback(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        0,
        &callbackData,
        nullptr
    );
    
    EXPECT_EQ(result, VK_FALSE);
}

TEST_F(ValidationLayersTest, DebugCallbackHandlesNullCallbackData) {
    // Test defensive programming - callback should handle null data gracefully
    VkBool32 result = ValidationLayerWrapper::debugCallback(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        0,
        nullptr, // null callback data
        nullptr
    );
    
    // Should still return VK_FALSE and not crash
    EXPECT_EQ(result, VK_FALSE);
}

TEST_F(ValidationLayersTest, DebugCallbackWithUserData) {
    // Test that user data parameter is properly handled
    int userData = 42;
    
    VkBool32 result = ValidationLayerWrapper::debugCallback(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        0,
        &callbackData,
        &userData
    );
    
    EXPECT_EQ(result, VK_FALSE);
}
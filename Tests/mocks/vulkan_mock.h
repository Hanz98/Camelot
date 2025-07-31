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

#ifndef TESTS_MOCKS_VULKAN_MOCK_H_
#define TESTS_MOCKS_VULKAN_MOCK_H_

#include <vulkan/vulkan.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/**
 * Mock interface for Vulkan operations to enable testing without real GPU
 */
class VulkanMock {
public:
    // Instance operations
    MOCK_METHOD(VkResult, CreateInstance, 
                (const VkInstanceCreateInfo* pCreateInfo,
                 const VkAllocationCallbacks* pAllocator,
                 VkInstance* pInstance));
    
    MOCK_METHOD(void, DestroyInstance,
                (VkInstance instance,
                 const VkAllocationCallbacks* pAllocator));
    
    // Physical device operations
    MOCK_METHOD(VkResult, EnumeratePhysicalDevices,
                (VkInstance instance,
                 uint32_t* pPhysicalDeviceCount,
                 VkPhysicalDevice* pPhysicalDevices));
    
    MOCK_METHOD(void, GetPhysicalDeviceProperties,
                (VkPhysicalDevice physicalDevice,
                 VkPhysicalDeviceProperties* pProperties));
    
    // Device operations
    MOCK_METHOD(VkResult, CreateDevice,
                (VkPhysicalDevice physicalDevice,
                 const VkDeviceCreateInfo* pCreateInfo,
                 const VkAllocationCallbacks* pAllocator,
                 VkDevice* pDevice));
    
    MOCK_METHOD(void, DestroyDevice,
                (VkDevice device,
                 const VkAllocationCallbacks* pAllocator));
    
    // Validation and debugging
    MOCK_METHOD(VkResult, CreateDebugUtilsMessengerEXT,
                (VkInstance instance,
                 const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                 const VkAllocationCallbacks* pAllocator,
                 VkDebugUtilsMessengerEXT* pMessenger));
    
    // Queue operations
    MOCK_METHOD(void, GetDeviceQueue,
                (VkDevice device,
                 uint32_t queueFamilyIndex,
                 uint32_t queueIndex,
                 VkQueue* pQueue));
};

// Global mock instance for testing
extern VulkanMock* g_vulkanMock;

/**
 * Test fixture for Vulkan-related tests
 */
class VulkanTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    
    VulkanMock vulkanMock;
};

#endif  // TESTS_MOCKS_VULKAN_MOCK_H_
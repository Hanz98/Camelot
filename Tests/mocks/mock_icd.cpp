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

#include "vulkan_mock.h"
#include <cstring>

// Global mock instance
VulkanMock* g_vulkanMock = nullptr;

void VulkanTestFixture::SetUp() {
    g_vulkanMock = &vulkanMock;
}

void VulkanTestFixture::TearDown() {
    g_vulkanMock = nullptr;
}

// Mock ICD implementation - provides a simple mock Vulkan driver
extern "C" {

// Instance functions
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    
    if (g_vulkanMock) {
        return g_vulkanMock->CreateInstance(pCreateInfo, pAllocator, pInstance);
    }
    
    // Default mock behavior
    *pInstance = reinterpret_cast<VkInstance>(0x12345678);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    
    if (g_vulkanMock) {
        g_vulkanMock->DestroyInstance(instance, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices) {
    
    if (g_vulkanMock) {
        return g_vulkanMock->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    }
    
    // Default mock behavior - return one mock device
    if (pPhysicalDevices == nullptr) {
        *pPhysicalDeviceCount = 1;
    } else {
        *pPhysicalDeviceCount = 1;
        pPhysicalDevices[0] = reinterpret_cast<VkPhysicalDevice>(0x87654321);
    }
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties) {
    
    if (g_vulkanMock) {
        g_vulkanMock->GetPhysicalDeviceProperties(physicalDevice, pProperties);
        return;
    }
    
    // Default mock properties
    memset(pProperties, 0, sizeof(VkPhysicalDeviceProperties));
    pProperties->apiVersion = VK_API_VERSION_1_3;
    pProperties->driverVersion = 1;
    pProperties->vendorID = 0x1002; // Mock vendor ID
    pProperties->deviceID = 0x1234; // Mock device ID
    pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    strcpy(pProperties->deviceName, "Mock Vulkan Device");
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    
    if (g_vulkanMock) {
        return g_vulkanMock->CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    }
    
    // Default mock behavior
    *pDevice = reinterpret_cast<VkDevice>(0x11223344);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    
    if (g_vulkanMock) {
        g_vulkanMock->DestroyDevice(device, pAllocator);
    }
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue) {
    
    if (g_vulkanMock) {
        g_vulkanMock->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
        return;
    }
    
    // Default mock behavior
    *pQueue = reinterpret_cast<VkQueue>(0x55667788);
}

// Add more Vulkan functions as needed for testing...

} // extern "C"
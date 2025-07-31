# Mock infrastructure for testing
add_library(MockVulkan INTERFACE)

target_include_directories(MockVulkan INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# For now, we'll create a simplified mock without actual Vulkan SDK dependency
# The mock ICD can be enhanced later when needed for integration tests
target_compile_definitions(MockVulkan INTERFACE MOCK_VULKAN_ENABLED)
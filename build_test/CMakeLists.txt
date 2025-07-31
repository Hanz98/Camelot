cmake_minimum_required(VERSION 3.15)

project(CamelotTests VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find GTest
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED gtest)
pkg_check_modules(GTEST_MAIN REQUIRED gtest_main)

# Include test directories
include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR}/../Avalon)

# Mock infrastructure
add_library(MockVulkan INTERFACE)
target_include_directories(MockVulkan INTERFACE ${CMAKE_SOURCE_DIR}/mocks)
target_compile_definitions(MockVulkan INTERFACE MOCK_VULKAN_ENABLED)

# Simple unit tests
add_executable(simple_tests 
    test_check_result_standalone.cpp
    ${CMAKE_SOURCE_DIR}/../Tests/unit/common/test_utilities.cpp
)

target_link_libraries(simple_tests 
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES}
    MockVulkan
    pthread
)

target_include_directories(simple_tests PRIVATE 
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../Avalon
    ${CMAKE_SOURCE_DIR}/mocks
)

enable_testing()
add_test(NAME SimpleTests COMMAND simple_tests)
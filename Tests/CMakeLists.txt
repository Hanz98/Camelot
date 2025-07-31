# Configure testing infrastructure
find_package(GTest REQUIRED)

# Add test directories
add_subdirectory(unit)
add_subdirectory(mocks)

# Main test executable with basic tests
add_executable(test_main test_main.cpp)
target_link_libraries(test_main PRIVATE GTest::gtest GTest::gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(test_main)
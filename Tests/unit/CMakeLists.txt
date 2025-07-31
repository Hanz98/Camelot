# Unit tests
find_package(GTest REQUIRED)

# Create unit test directories
add_subdirectory(avalon)
add_subdirectory(camelot)
add_subdirectory(common)

# Main unit test suite executable
set(UNIT_TEST_SOURCES
    test_runner.cpp
)

add_executable(unit_tests ${UNIT_TEST_SOURCES})

target_link_libraries(unit_tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    AvalonTests
    CamelotTests
    CommonTests
    MockVulkan
    pthread
)

# Include directories for accessing source code
target_include_directories(unit_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Avalon
    ${CMAKE_SOURCE_DIR}/Camelot
    ${CMAKE_SOURCE_DIR}/Tests/mocks
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
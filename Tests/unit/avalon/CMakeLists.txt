# Avalon module unit tests

set(AVALON_TEST_SOURCES
    test_check_result.cpp
    test_validation_layers.cpp
    test_instance.cpp
    test_device.cpp
)

add_library(AvalonTests STATIC ${AVALON_TEST_SOURCES})

target_link_libraries(AvalonTests PRIVATE
    GTest::gtest
    MockVulkan
)

target_include_directories(AvalonTests PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Avalon
    ${CMAKE_SOURCE_DIR}/Tests/mocks
)
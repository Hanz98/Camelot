add_executable(test_main 
    test_main.cpp
    test_mcap_reader.cpp
)

find_package(GTest REQUIRED)
target_link_libraries(test_main PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    pthread
    Pendragon_data
)

target_include_directories(test_main PRIVATE ${PROJECT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(test_main)
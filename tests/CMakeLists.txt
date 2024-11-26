add_executable(test_main test_main.cpp)

find_package(GTest REQUIRED)
target_link_libraries(test_main gtest::gtest)

include(GoogleTest)
gtest_discover_tests(test_main)

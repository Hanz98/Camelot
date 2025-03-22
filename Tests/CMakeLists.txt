# In Tests/CMakeLists.txt
add_executable(Test_Main test_main.cpp)

find_package(GTest REQUIRED)

# Include directories for all tests
target_include_directories(Test_Main PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Test_Main
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        Avalon
)

include(GoogleTest)
gtest_discover_tests(Test_Main)

# Add all test source files
add_subdirectory(Avalon)

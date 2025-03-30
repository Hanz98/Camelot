# In Tests/CMakeLists.txt

enable_testing()

find_package(GTest REQUIRED)

add_executable(Test_Main test_main.cpp)


# Include directories for all tests
target_include_directories(Test_Main PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_subdirectory("ext")
add_subdirectory(Avalon)

target_link_libraries(Test_Main
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        Avalon
)


include(GoogleTest)
gtest_discover_tests(Test_Main)

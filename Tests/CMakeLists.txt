add_executable(test_main test_main.cpp)

find_package(GTest REQUIRED)


IF (WIN32)
	target_link_libraries(test_main PRIVATE GTest::gtest_main)
ELSE()
	target_link_libraries(test_main PRIVATE GTest::gtest GTest::gtest_main pthread)
ENDIF()

include(GoogleTest)
gtest_discover_tests(test_main)
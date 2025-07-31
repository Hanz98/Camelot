cmake_minimum_required(VERSION 3.15)
project(CamelotTestRunner VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED gtest)
pkg_check_modules(GTEST_MAIN REQUIRED gtest_main)

# Simple validation tests
add_executable(validation_tests 
    ${CMAKE_SOURCE_DIR}/../build_test/test_check_result_standalone.cpp
)

target_link_libraries(validation_tests 
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

target_include_directories(validation_tests PRIVATE 
    ${GTEST_INCLUDE_DIRS}
)

enable_testing()
add_test(NAME ValidationTests COMMAND validation_tests)

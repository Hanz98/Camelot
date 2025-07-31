# Common utility tests

set(COMMON_TEST_SOURCES
    test_utilities.cpp
)

add_library(CommonTests STATIC ${COMMON_TEST_SOURCES})

target_link_libraries(CommonTests PRIVATE
    GTest::gtest
)

target_include_directories(CommonTests PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Avalon
)
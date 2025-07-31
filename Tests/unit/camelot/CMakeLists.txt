# Camelot module unit tests

set(CAMELOT_TEST_SOURCES
    test_camelot_core.cpp
)

add_library(CamelotTests STATIC ${CAMELOT_TEST_SOURCES})

target_link_libraries(CamelotTests PRIVATE
    GTest::gtest
)

target_include_directories(CamelotTests PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Camelot
)
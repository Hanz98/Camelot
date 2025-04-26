add_library(Test_Avalon_Window
    WindowTest.cpp
)

find_package(GTest REQUIRED)

target_link_libraries(Test_Avalon_Window
    PRIVATE  # Changed from PUBLIC to PRIVATE since tests are end targets
        Avalon
        GTest::gtest
        GTest::gtest_main
        pthread
)

# Add the test to be discovered by CTest
add_test(NAME Test_Avalon_Window COMMAND Test_Avalon_Window)

# If you need to add this test source to a global collection
target_sources(Test_Main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/WindowTest.cpp
)

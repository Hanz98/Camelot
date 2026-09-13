# Unit tests. Every test source is added to the single Test_Main executable
# through target_sources() in the subdirectories below.

option(CAMELOT_TESTS_USE_MOCK_ICD
       "Run the tests against the bundled mock Vulkan ICD instead of a real GPU"
       OFF)

find_package(GTest REQUIRED)
find_package(vk-bootstrap REQUIRED)

add_executable(Test_Main test_main.cpp)

target_include_directories(Test_Main PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(Test_Main
    PRIVATE
        GTest::gtest
        Avalon
        glfw
        vk-bootstrap::vk-bootstrap
)

add_subdirectory(ext)
add_subdirectory(Avalon)

set(CAMELOT_TEST_ENVIRONMENT "")
if(CAMELOT_TESTS_USE_MOCK_ICD)
    set(_mock_icd_json "${CMAKE_BINARY_DIR}/lib/VkICD_mock_icd.json")
    # VK_DRIVER_FILES is the current loader variable, VK_ICD_FILENAMES the
    # legacy one; both point at the mock driver so only it is loaded.
    list(APPEND CAMELOT_TEST_ENVIRONMENT
         "VK_DRIVER_FILES=${_mock_icd_json}"
         "VK_ICD_FILENAMES=${_mock_icd_json}"
         "CAMELOT_TESTS_USE_MOCK_ICD=1")
    add_dependencies(Test_Main VkICD_mock_icd)
endif()

include(GoogleTest)
gtest_discover_tests(Test_Main
    PROPERTIES ENVIRONMENT "${CAMELOT_TEST_ENVIRONMENT}"
)

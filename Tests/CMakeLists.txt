# Unit tests. Every test source is added to the single Test_Main executable
# through target_sources() in the subdirectories below.

option(CAMELOT_TESTS_USE_MOCK_ICD
       "Run the tests against the bundled mock Vulkan ICD instead of a real GPU"
       OFF)

find_package(GTest REQUIRED)
find_package(glfw3 REQUIRED)
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
add_subdirectory(Camelot)

if(CAMELOT_TESTS_USE_MOCK_ICD)
  # The test binary points the Vulkan loader at the mock driver itself (see
  # test_main.cpp). Passing the variables through CTest's ENVIRONMENT
  # property is not reliable across CMake versions: with CMake 3.31 the list
  # arrived as one string and CI silently ran on whatever driver the runner
  # had. CAMELOT_MOCK_ICD_JSON is set by Tests/ext/CMakeLists.txt: the manifest
  # sits next to the mock driver library (lib/ on Linux, bin/ on Windows).
  if(NOT CAMELOT_MOCK_ICD_JSON)
    message(FATAL_ERROR "CAMELOT_MOCK_ICD_JSON not set by Tests/ext")
  endif()
  target_compile_definitions(
    Test_Main PRIVATE CAMELOT_MOCK_ICD_JSON="${CAMELOT_MOCK_ICD_JSON}")
  add_dependencies(Test_Main VkICD_mock_icd)
endif()

# The suite is run as one GoogleTest process: `bin/Test_Main` (filters, XML
# output and the rest come from gtest flags). No gtest_discover_tests(): it
# launches the binary at build time and was the layer that dropped the test
# environment on CI. The single add_test() keeps `ctest` and IDE test
# explorers working.
add_test(NAME Test_Main COMMAND Test_Main)

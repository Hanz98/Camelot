cmake_minimum_required(VERSION 3.15)

project(Camelot VERSION 1.0)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

#set(Camelot_external_dirs ${PROJECT_SOURCE_DIR}/external/)
set(Camelot_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)

# Try to build Avalon if all dependencies are available
if(DEFINED ENV{VULKAN_SDK})
    # Check for vk-bootstrap and volk availability
    find_package(vk-bootstrap QUIET)
    find_package(volk QUIET)
    find_package(glfw3 QUIET)
    find_package(spdlog QUIET)
    
    if(vk-bootstrap_FOUND AND volk_FOUND AND glfw3_FOUND AND spdlog_FOUND)
        add_subdirectory(Avalon)
        add_subdirectory(Camelot)
        message(STATUS "Building Avalon and Camelot with all dependencies")
    else()
        message(STATUS "Skipping Avalon - missing dependencies (vk-bootstrap: ${vk-bootstrap_FOUND}, volk: ${volk_FOUND}, glfw3: ${glfw3_FOUND}, spdlog: ${spdlog_FOUND})")
        message(STATUS "Skipping Camelot - depends on Avalon")
    endif()
else()
    message(STATUS "Skipping Avalon - VULKAN_SDK not found")
    message(STATUS "Skipping Camelot - depends on Avalon")
endif()
#add_subdirectory(Excalibur)
#add_subdirectory(Merlin )
#add_subdirectory(Nimue)
add_subdirectory(Pendragon)

# Try to build Tests if GTest is available
find_package(GTest QUIET)
if(GTest_FOUND)
    add_subdirectory(Tests)
else()
    message(STATUS "Skipping Tests - GTest not found")
endif()

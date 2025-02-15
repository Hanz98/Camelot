
IF(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    ELSEIF(NOT DEFINED VULKAN_SDK OR VULKAN_SDK STREQUAL "NOT-SET")
SET(VULKAN_SDK "NOT-SET" CACHE PATH "VULKAN_SDK directory")
    MESSAGE( FATAL_ERROR "** Please add the variable 'VULKAN_SDK'")
ENDIF()

set(Avalon_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})


find_package(glfw3 REQUIRED)
find_package(glog REQUIRED)

include_directories(${VULKAN_SDK_PATH}/include/)
include_directories(${glfw3_INCLUDE_DIRS}/include/)
#include_directories(${glog_INCLUDE_DIRS}/include)
include_directories(${Camelot_include_dirs})

#add_subdirectory("API")
add_subdirectory("src")


add_library(Avalon INTERFACE)
target_include_directories(Avalon INTERFACE ${glfw3_INCLUDE_DIRS})
target_include_directories(Avalon INTERFACE ${VULKAN_SDK_PATH}/include/)

target_link_libraries(Avalon INTERFACE glog::glog)
target_link_libraries(Avalon INTERFACE Avalon_src)
target_link_libraries(Avalon INTERFACE Avalon_window)

target_precompile_headers(Avalon INTERFACE pch.h)

message("Camelot_include_dirs: ${Camelot_include_dirs}")
message("glfw3_INCLUDE_DIRS: ${glfw3_INCLUDE_DIRS}")
message(STATUS "VULKAN_SDK environment variable: $ENV{VULKAN_SDK}")
message("Current source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message("Camelot_include_dirs: ${Camelot_include_dirs}")

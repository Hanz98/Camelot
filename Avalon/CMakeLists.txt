
IF(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    ELSEIF(NOT DEFINED VULKAN_SDK OR VULKAN_SDK STREQUAL "NOT-SET")
SET(VULKAN_SDK "NOT-SET" CACHE PATH "VULKAN_SDK directory")
    MESSAGE( FATAL_ERROR "** Please add the variable 'VULKAN_SDK'")
ENDIF()

set(Avalon_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

add_compile_definitions(VK_NO_PROTOTYPES)

find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(volk REQUIRED)

include_directories(${VULKAN_SDK_PATH}/include/)
include_directories(${glfw3_INCLUDE_DIRS}/include/)
include_directories(${spdlog_INCLUDE_DIRS})
include_directories(${vk-bootstrap_INCLUDE_DIRS})
include_directories(${Camelot_include_dirs})
include_directories(${volk_INCLUDE_DIRS})

# ...
#add_subdirectory("API")
add_subdirectory("src")


add_library(Avalon INTERFACE)

target_compile_definitions(Avalon INTERFACE VK_NO_PROTOTYPES)

target_include_directories(Avalon INTERFACE ${glfw3_INCLUDE_DIRS})
target_include_directories(Avalon INTERFACE ${VULKAN_SDK_PATH}/include/)

target_link_libraries(Avalon INTERFACE Avalon_src)
target_link_libraries(Avalon INTERFACE Avalon_window)
target_link_libraries(Avalon INTERFACE spdlog::spdlog)
target_link_libraries(Avalon INTERFACE volk::volk)

target_precompile_headers(Avalon INTERFACE "pch.h")

message("volk_INCLUDE_DIRS: ${volk_INCLUDE_DIRS}")

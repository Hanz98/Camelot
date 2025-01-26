
IF(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    ELSEIF(NOT DEFINED VULKAN_SDK OR VULKAN_SDK STREQUAL "NOT-SET")
SET(VULKAN_SDK "NOT-SET" CACHE PATH "VULKAN_SDK directory")
    MESSAGE( FATAL_ERROR "** Please add the variable 'VULKAN_SDK'")
ENDIF()

set(Avalon_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})


include_directories(${VULKAN_SDK_PATH}/include)

#add_subdirectory("API")
add_subdirectory("src")


add_library(Avalon INTERFACE)
target_include_directories(Avalon INTERFACE ${VULKAN_SDK_PATH}/include)

target_precompile_headers(Avalon INTERFACE pch.h)

message("Camelot_external_dirs: ${VULKAN_SDK_PATH}/include")
message(STATUS "VULKAN_SDK environment variable: $ENV{VULKAN_SDK}")

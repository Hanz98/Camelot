
IF(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    ELSEIF(NOT DEFINED VULKAN_SDK OR VULKAN_SDK STREQUAL "NOT-SET")
SET(VULKAN_SDK "NOT-SET" CACHE PATH "VULKAN_SDK directory")
    MESSAGE( FATAL_ERROR "** Please add the variable 'VULKAN_SDK'")
ENDIF()

set(Avalon_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})


find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(volk REQUIRED)

# ...
#add_subdirectory("API")
add_subdirectory("src")

add_library(Avalon "main.cpp")


target_include_directories(Avalon
    PUBLIC
        ${VULKAN_SDK_PATH}/include/
        ${glfw3_INCLUDE_DIRS}/include/
        ${spdlog_INCLUDE_DIRS}
        ${vk-bootstrap_INCLUDE_DIRS}
        ${volk_INCLUDE_DIRS}
)

target_compile_definitions(Avalon PUBLIC VK_NO_PROTOTYPES)

target_link_libraries(Avalon
    PUBLIC
        Avalon_src
        Avalon_window
        Avalon_main
        spdlog::spdlog
    PRIVATE
        volk::volk
)

target_compile_definitions(Avalon
    PUBLIC
        VK_NO_PROTOTYPES
)

target_precompile_headers(Avalon PUBLIC "pch.h")

# The Vulkan headers, loader and VMA come from Conan. VULKAN_SDK is only used
# as an additional include hint when it is set in the environment.
if(DEFINED ENV{VULKAN_SDK})
  set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
endif()

set(Avalon_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)

# GLSL sources under shaders/ are compiled to SPIR-V and embedded into the
# Avalon_shaders library; look them up through <Avalon/shaders/Registry.h>.
camelot_add_shaders(Avalon_shaders
    NAMESPACE avalon::shaders
    HEADER Avalon/shaders/Registry.h
    TARGET_ENV vulkan1.1
    SOURCES
        shaders/triangle.vert
        shaders/triangle.frag
)

add_subdirectory("interface")
add_subdirectory("src")

add_library(Avalon "main.cpp")

target_include_directories(Avalon
    PUBLIC
        ${glfw3_INCLUDE_DIRS}/include/
        ${spdlog_INCLUDE_DIRS}
        ${vk-bootstrap_INCLUDE_DIRS}
)
if(DEFINED VULKAN_SDK_PATH)
  target_include_directories(Avalon PUBLIC ${VULKAN_SDK_PATH}/include/)
endif()

target_link_libraries(Avalon
    PUBLIC
        Avalon_src
        Avalon_window
        Avalon_main
        spdlog::spdlog
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
)

target_precompile_headers(Avalon PUBLIC "pch.h")

# Third-party test helpers: the Khronos mock ICD (a GPU-less null driver).

# The vendored ICD CMake links against the upstream Vulkan-Headers target
# name. Conan's VulkanHeaders package declares it as well, but map it from
# the modern Vulkan::Headers target when only that one exists.
find_package(VulkanHeaders REQUIRED)
if(NOT TARGET vulkan-headers::vulkan-headers AND TARGET Vulkan::Headers)
    add_library(vulkan-headers::vulkan-headers ALIAS Vulkan::Headers)
endif()

# The mock must create real (fake-handle) surfaces for the window system GLFW
# uses, otherwise Vulkan loaders since 1.4 report no presentation support for
# the loader-owned surface and device selection fails.
if(UNIX AND NOT APPLE)
  set(BUILD_WSI_XCB_SUPPORT ON)
  set(BUILD_WSI_XLIB_SUPPORT ON)
endif()

add_subdirectory(icd)

# The manifest references the shared library relative to its own location,
# so it is placed next to libVkICD_mock_icd in CMAKE_LIBRARY_OUTPUT_DIRECTORY.
if(WIN32)
    set(_icd_manifest ${CMAKE_CURRENT_SOURCE_DIR}/icd/windows/VkICD_mock_icd.json)
elseif(APPLE)
    set(_icd_manifest ${CMAKE_CURRENT_SOURCE_DIR}/icd/macos/VkICD_mock_icd.json)
elseif(UNIX)
    set(_icd_manifest ${CMAKE_CURRENT_SOURCE_DIR}/icd/linux/VkICD_mock_icd.json)
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()
configure_file(${_icd_manifest} ${CMAKE_BINARY_DIR}/lib/VkICD_mock_icd.json COPYONLY)

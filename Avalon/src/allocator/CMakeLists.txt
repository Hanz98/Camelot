add_library(
    Avalon_allocator
            "VmaAllocator.h"
            "VmaAllocator.cpp"
)

target_include_directories(Avalon_allocator PUBLIC ${Avalon_include_dirs})
target_include_directories(Avalon_allocator PUBLIC ${glfw3_INCLUDE_DIRS})

target_link_libraries(Avalon_allocator PUBLIC  Avalon
                                            Avalon_interface_window
                                            glfw)
